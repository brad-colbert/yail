CC65_TARGET = atari

ifneq ($(shell echo),)
  CMD_EXE = 1
endif

ifdef CMD_EXE
  NULLDEV = nul:
  DEL = -del /f
  RMDIR = rmdir /s /q
else
  NULLDEV = /dev/null
  DEL = $(RM)
  RMDIR = $(RM) -r
endif

SOURCES = imgload.c displaylist.c

PROGRAM = imgload

ifdef CC65_TARGET
CC      = cl65
CFLAGS  = -t $(CC65_TARGET) --create-dep $(<:.c=.d) -O
LDFLAGS = -t $(CC65_TARGET) -m $(PROGRAM).map
else
CC      = gcc
CFLAGS  = -MMD -MP -O
LDFLAGS = -Wl,-Map,$(PROGRAM).map
endif

########################################

DIR2ATR ?= dir2atr

DISK     = imgload.atr

########################################

.SUFFIXES:
.PHONY: all clean

disk: $(DISK)

all: $(PROGRAM)

ifneq ($(MAKECMDGOALS),clean)
-include $(SOURCES:.c=.d)
endif

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROGRAM): $(SOURCES:.c=.o)
	$(CC) $(LDFLAGS) -o $@ $^

########################################

define ATR_WRITE_recipe

cp $(file) atr/$(notdir $(file))

endef # ATR_WRITE_recipe

$(DISK): $(PROGRAM)
	@mkdir atr
	cp dos.sys atr/dos.sys
	cp dup.sys atr/dup.sys
	cp mtfujid.pbm atr/mtfujid.pbm
	cp mtfuji.pgm atr/mtfuji.pgm
	@$(foreach file,$(PROGRAM),$(ATR_WRITE_recipe))
	$(DIR2ATR) -S -b Dos25 $@ atr
	@$(RMDIR) atr

clean:
	$(RM) $(SOURCES:.c=.o) $(SOURCES:.c=.d) $(PROGRAM) $(PROGRAM).map $(DISK)
