/*
 * wokwi-pzem-004t — Modbus RTU energy meter custom chip for Wokwi.
 *
 * Speaks Modbus RTU on UART (default 9600 8N1, slave id 1) and answers
 * function code 04 (read input registers) — the same flow as a real
 * PZEM-004T v3. The register map deliberately matches our gateway's
 * default config so swapping the chip in is a drop-in replacement for
 * the on-chip WokwiSlave simulation:
 *
 *   addr 0x0000  uint16  voltage      0.1   V/LSB
 *   addr 0x0001  uint32  current      0.001 A/LSB  (big-endian word order)
 *   addr 0x0003  uint32  power        0.1   W/LSB
 *   addr 0x0005  int16   temperature  0.1   C/LSB
 *
 * Values are sinusoidal with light noise so dashboards look alive.
 *
 * Frame separation uses the standard ~3.5 character-time silence rule:
 * every received byte restarts an inter-frame timer; when it fires we
 * treat whatever is in the buffer as one complete frame, validate
 * slave-id + CRC, and reply.
 */

#include "wokwi-api.h"

#define DEFAULT_SLAVE_ID  1
#define DEFAULT_BAUD      9600
#define INTERFRAME_US     4000   // ~3.5 char times at 9600 baud
#define MAX_FRAME         256

typedef struct {
    uart_dev_t uart;
    timer_t inter_frame_timer;
    timer_t boot_clock;
    uint8_t slave_id;

    uint8_t  rx_buf[MAX_FRAME];
    uint16_t rx_len;

    uint32_t boot_ms;     // monotonic ms since chip start
    uint32_t tick_ms;
} pzem_t;

// --- Helpers ----------------------------------------------------------------

static uint16_t crc16_modbus(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

// Cheap sin approximation good enough for waving a dashboard around.
// 5th-order Taylor on the symmetric domain, accurate to ~0.001.
static float fsin(float x) {
    // Range-reduce to [-pi, pi]
    const float PI = 3.1415926f;
    while (x >  PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f + x2 * x2 / 120.0f - x2 * x2 * x2 / 5040.0f);
}

static uint32_t lcg_state = 1;
static float frand(float lo, float hi) {
    lcg_state = lcg_state * 1103515245u + 12345u;
    float r = ((lcg_state >> 16) & 0x7FFF) / 32767.0f;
    return lo + r * (hi - lo);
}

// --- Register sampler -------------------------------------------------------

// Computes a 16-bit register value at the requested logical address.
static uint16_t sample_register(pzem_t *pzem, uint16_t addr) {
    float t = pzem->tick_ms / 1000.0f;

    uint16_t voltage_dV    = (uint16_t)((230.0f + 2.5f * fsin(t / 7.0f)
                                                + frand(-0.5f, 0.5f)) * 10.0f);
    uint32_t current_mA    = (uint32_t)(2300.0f + 400.0f * fsin(t / 11.0f)
                                                + frand(-50.0f, 50.0f));
    uint32_t power_dW      = (uint32_t)((float)voltage_dV * current_mA
                                        / 10000.0f * 10.0f);
    int16_t  temp_dC       = (int16_t)((23.0f + 3.0f * fsin(t / 60.0f)
                                                + frand(-0.2f, 0.2f)) * 10.0f);

    switch (addr) {
        case 0x0000: return voltage_dV;
        case 0x0001: return (uint16_t)((current_mA >> 16) & 0xFFFF);
        case 0x0002: return (uint16_t)(current_mA & 0xFFFF);
        case 0x0003: return (uint16_t)((power_dW   >> 16) & 0xFFFF);
        case 0x0004: return (uint16_t)(power_dW    & 0xFFFF);
        case 0x0005: return (uint16_t)temp_dC;
        default:     return 0;
    }
}

// --- Frame handling ---------------------------------------------------------

static void send_exception(pzem_t *pzem, uint8_t fc, uint8_t code) {
    uint8_t resp[5];
    resp[0] = pzem->slave_id;
    resp[1] = fc | 0x80;
    resp[2] = code;
    uint16_t crc = crc16_modbus(resp, 3);
    resp[3] = (uint8_t)(crc & 0xFF);
    resp[4] = (uint8_t)(crc >> 8);
    uart_write(pzem->uart, resp, 5);
}

static void handle_read_input(pzem_t *pzem) {
    uint16_t addr  = ((uint16_t)pzem->rx_buf[2] << 8) | pzem->rx_buf[3];
    uint16_t count = ((uint16_t)pzem->rx_buf[4] << 8) | pzem->rx_buf[5];

    if (count == 0 || count > 0x7D) {
        send_exception(pzem, 0x04, 0x03);  // illegal data value
        return;
    }
    if (addr + count > 0x0010) {
        send_exception(pzem, 0x04, 0x02);  // illegal data address
        return;
    }

    uint8_t resp[256];
    resp[0] = pzem->slave_id;
    resp[1] = 0x04;
    resp[2] = (uint8_t)(count * 2);

    for (uint16_t i = 0; i < count; i++) {
        uint16_t v = sample_register(pzem, addr + i);
        resp[3 + 2 * i + 0] = (uint8_t)(v >> 8);
        resp[3 + 2 * i + 1] = (uint8_t)(v & 0xFF);
    }

    uint16_t resp_len = 3 + count * 2;
    uint16_t crc = crc16_modbus(resp, resp_len);
    resp[resp_len + 0] = (uint8_t)(crc & 0xFF);
    resp[resp_len + 1] = (uint8_t)(crc >> 8);

    uart_write(pzem->uart, resp, resp_len + 2);
}

static void on_inter_frame(void *user_data) {
    pzem_t *pzem = (pzem_t *)user_data;
    if (pzem->rx_len < 4) {
        pzem->rx_len = 0;
        return;
    }

    // Slave ID check.
    if (pzem->rx_buf[0] != pzem->slave_id) {
        pzem->rx_len = 0;
        return;
    }

    // CRC check.
    uint16_t recv_crc = ((uint16_t)pzem->rx_buf[pzem->rx_len - 1] << 8)
                       |  (uint16_t)pzem->rx_buf[pzem->rx_len - 2];
    uint16_t calc_crc = crc16_modbus(pzem->rx_buf, pzem->rx_len - 2);
    if (recv_crc != calc_crc) {
        pzem->rx_len = 0;
        return;
    }

    uint8_t fc = pzem->rx_buf[1];
    if (fc == 0x04 || fc == 0x03) {  // treat holding read identically
        handle_read_input(pzem);
    } else {
        send_exception(pzem, fc, 0x01);  // illegal function
    }
    pzem->rx_len = 0;
}

static void on_rx(void *user_data, uint8_t byte) {
    pzem_t *pzem = (pzem_t *)user_data;
    if (pzem->rx_len < MAX_FRAME) {
        pzem->rx_buf[pzem->rx_len++] = byte;
    }
    // Restart inter-frame timer; when it fires the buffer is one frame.
    timer_start(pzem->inter_frame_timer, INTERFRAME_US, false);
}

// --- Boot clock — ticks every millisecond ----------------------------------

static void on_boot_tick(void *user_data) {
    pzem_t *pzem = (pzem_t *)user_data;
    pzem->tick_ms++;
}

// --- Entry point ------------------------------------------------------------

// Single static instance — Wokwi only spawns one of each custom chip per
// diagram, and we avoid needing a libc / malloc by skipping calloc.
static pzem_t g_pzem;

void chip_init(void) {
    pzem_t *pzem = &g_pzem;
    pzem->slave_id = DEFAULT_SLAVE_ID;

    uart_config_t uart_cfg = {
        .rx = pin_init("RX", INPUT),
        .tx = pin_init("TX", OUTPUT),
        .baud_rate = DEFAULT_BAUD,
        .rx_data = on_rx,
        .user_data = pzem,
    };
    pzem->uart = uart_init(&uart_cfg);

    timer_config_t frame_cfg = {
        .callback = on_inter_frame,
        .user_data = pzem,
    };
    pzem->inter_frame_timer = timer_init(&frame_cfg);

    timer_config_t clock_cfg = {
        .callback = on_boot_tick,
        .user_data = pzem,
    };
    pzem->boot_clock = timer_init(&clock_cfg);
    timer_start(pzem->boot_clock, 1000, true);  // 1ms periodic
}
