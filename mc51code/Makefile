ASEM ?= asem

SRCS := blink_p1.asm blink_delay.asm counter_ram.asm add_loop.asm call_ret.asm serial_hello.asm bas52.asm
HEXS := $(SRCS:.asm=.hex)

.PHONY: all clean

all: $(HEXS)

%.hex: %.asm
	$(ASEM) $<

clean:
	rm -f $(HEXS) *.lst *.map *.sym
