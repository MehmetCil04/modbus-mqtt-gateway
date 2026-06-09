/*
 * Minimal subset of the Wokwi Custom Chips C API used by this chip.
 * Full reference: https://docs.wokwi.com/chips-api/getting-started
 *
 * The Wokwi runtime exposes these as imported WebAssembly functions, so
 * we only declare the signatures here — no implementation.
 */

#ifndef WOKWI_API_H
#define WOKWI_API_H

#include <stdint.h>

typedef uint32_t pin_t;
typedef uint32_t timer_t;
typedef uint32_t uart_dev_t;

#define INPUT          0
#define OUTPUT         1
#define INPUT_PULLUP   2
#define INPUT_PULLDOWN 3
#define ANALOG         4

#define LOW  0
#define HIGH 1

typedef struct {
    void (*callback)(void *user_data);
    void *user_data;
} timer_config_t;

typedef struct {
    pin_t rx;
    pin_t tx;
    uint32_t baud_rate;
    void (*rx_data) (void *user_data, uint8_t byte);
    void (*write_done) (void *user_data);
    void *user_data;
} uart_config_t;

// Pin API
pin_t pin_init(const char *name, uint32_t mode);
uint8_t pin_read(pin_t pin);
void pin_write(pin_t pin, uint8_t value);

// Timer API — delays expressed in microseconds.
timer_t timer_init(const timer_config_t *config);
void timer_start(timer_t timer, uint32_t delay_us, _Bool periodic);
void timer_stop(timer_t timer);

// UART API
uart_dev_t uart_init(const uart_config_t *config);
void uart_write(uart_dev_t uart, const uint8_t *data, uint32_t count);

#endif  // WOKWI_API_H
