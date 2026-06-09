# Build the Wokwi PZEM-004T custom chip to WebAssembly.
#
# Requires a clang with wasm32 target. Two ways to get one on Windows:
#
#   A) WASI SDK — pre-built bundle (~150MB, no installer):
#        Invoke-WebRequest -OutFile $env:USERPROFILE\wasi-sdk-33.tar.gz \
#          https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-33/wasi-sdk-33.0-x86_64-windows.tar.gz
#        tar -xzf $env:USERPROFILE\wasi-sdk-33.tar.gz -C $env:USERPROFILE
#        # Now $env:USERPROFILE\wasi-sdk-33.0-x86_64-windows\bin\clang.exe is available.
#
#   B) Full LLVM (~700MB installer):
#        winget install LLVM.LLVM
#        # Add C:\Program Files\LLVM\bin to PATH.
#
# Usage:
#   make           # builds chip.wasm
#   make clean     # removes build outputs
#
# Override clang location:
#   make CLANG=/path/to/clang

CLANG ?= clang

TARGET  = chip.wasm
SOURCES = src/main.c

CFLAGS  = -Wall -Wno-unused-parameter \
          -mcpu=mvp -nostdlib \
          -Wl,--no-entry,--export-table,--initial-memory=65536,--max-memory=65536,--stack-first,-zstack-size=8192

$(TARGET): $(SOURCES)
	$(CLANG) --target=wasm32 $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: clean
