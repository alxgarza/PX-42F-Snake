# A PX-42F app or game. Change these two for your own:
NAME := SNAKE# the file: build/SNAKE.bin, shown in the menu as SNAKE
KIND := GAMES# GAMES or APPS: the folder it goes in, /BIN/GAMES or /BIN/APPS

# The simulator's key script (sim/sim.c): keys, waits in ms, shot:NAME.
SIM_SCRIPT := shot:title ENTER 1500 shot:playing XEQ 900 SIGMA 2000 shot:later RS shot:paused RS \
              3000 XEQ 190 XEQ 190 XEQ 1000 shot:end

PICO_TOOLCHAIN ?= $(HOME)/.pico-sdk/toolchain/15_2_Rel1
DRIVE ?= /Volumes/PX-42F

CC := $(PICO_TOOLCHAIN)/bin/arm-none-eabi-gcc
OBJCOPY := $(PICO_TOOLCHAIN)/bin/arm-none-eabi-objcopy
NM := $(PICO_TOOLCHAIN)/bin/arm-none-eabi-nm
SIZE := $(PICO_TOOLCHAIN)/bin/arm-none-eabi-size

SRC := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h) $(wildcard px42f/*.h)

# The firmware's: Cortex-M33, Thumb, floats in integer registers (softfp).
ARCH := -mcpu=cortex-m33 -mthumb -march=armv8-m.main+fp+dsp -mfloat-abi=softfp
CFLAGS := $(ARCH) -std=c11 -Os -Wall -Wextra -ffunction-sections -fdata-sections \
          -fno-unwind-tables -fno-asynchronous-unwind-tables -Ipx42f -Isrc
LDFLAGS := $(ARCH) -T px42f/app.ld -nostartfiles --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,--orphan-handling=error -Wl,--no-warn-rwx-segments -Wl,-Map=build/$(NAME).map

.PHONY: all sim install clean

all: build/$(NAME).bin

build/$(NAME).elf: $(SRC) px42f/runtime.c px42f/app.ld $(HEADERS) | build
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) px42f/runtime.c -o $@
	$(SIZE) $@

build/$(NAME).bin: build/$(NAME).elf px42f/pxbin.py
	$(OBJCOPY) -O binary -j .image $< build/$(NAME).img
	$(NM) $< > build/$(NAME).sym
	python3 px42f/pxbin.py build/$(NAME).img build/$(NAME).sym $@

# The simulator: the same app code on the Mac. Screenshots in build/sim-out;
# the drive is a copy of sim/drive, in build/sim-drive.
build/sim: $(SRC) $(HEADERS) $(wildcard sim/*.c sim/*.h sim/fonts/*) | build
	cc -std=c11 -g -Wall -Wextra -fsanitize=address,undefined -Ipx42f -Isrc -Isim \
	   -DSIM_APP_DIR='"/BIN/$(KIND)/$(NAME)"' \
	   $(SRC) sim/*.c sim/fonts/*.c -o $@

sim: build/sim
	rm -rf build/sim-out build/sim-drive
	mkdir -p build/sim-drive && cp -R sim/drive/. build/sim-drive/
	build/sim build/sim-out build/sim-drive $(SIM_SCRIPT)

# Onto the calculator's drive: Settings -> USB Drive -> Connect to Computer.
install: build/$(NAME).bin
	@test -d "$(DRIVE)/BIN/$(KIND)" || { echo "$(DRIVE)/BIN/$(KIND) not found: connect the PX-42F as a USB drive"; exit 1; }
	cp build/$(NAME).bin "$(DRIVE)/BIN/$(KIND)/"
	@echo "Copied. Eject the drive, then Settings -> Games & Apps."

build:
	mkdir -p build

clean:
	rm -rf build
