TARGETMCU	?= msp430g2553

CROSS		:= msp430-
CC		:= $(CROSS)gcc
MSPDEBUG	:= mspdebug
CFLAGS		:= -Os -Wall -Werror -g -mmcu=$(TARGETMCU)
CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections

PROG			:= wiznet_test

all:			$(PROG).elf

$(PROG).elf:	$(OBJS)
	$(CC) $(CFLAGS) -o $(PROG).elf *.c

clean:
	-rm -f $(PROG).elf

install: $(PROG).elf
	$(MSPDEBUG) -n rf2500 "prog $(PROG).elf"
