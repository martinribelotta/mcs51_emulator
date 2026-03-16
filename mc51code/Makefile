ASEM ?= asem

SRCS := blink_p1.asm counter_ram.asm add_loop.asm call_ret.asm
HEXS := $(SRCS:.asm=.hex)

.PHONY: all clean

all: $(HEXS)

%.hex: %.asm
	$(ASEM) $<

clean:
	rm -f $(HEXS) *.lst *.map *.sym
