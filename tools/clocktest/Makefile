# The CX II clock test (see clocktest.c):
#   . ~/nspire/build/env.sh && make -C tools/clocktest
# makes pocketsnes_clocktest.tns.
TARGET  = pocketsnes_clocktest
CC      = nspire-gcc
LD      = nspire-ld
CFLAGS  = -Os -Wall -Wextra -marm
ZEHNFLAGS = --name "PocketSNES clock test" --240x320-support true --ndless-min 31

all: $(TARGET).tns

$(TARGET).elf: clocktest.c
	$(CC) $(CFLAGS) clocktest.c -o $@

$(TARGET).tns: $(TARGET).elf
	genzehn --input $< --output $(TARGET).zehn $(ZEHNFLAGS)
	make-prg $(TARGET).zehn $@
	rm -f $(TARGET).zehn

clean:
	rm -f $(TARGET).elf $(TARGET).zehn $(TARGET).tns

.PHONY: all clean
