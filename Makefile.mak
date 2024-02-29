CC65_HOME=C:\\Users\\bradc\\AppData\\Roaming\\cc65\\bin
CC65=$(CC65_HOME)\\cc65.exe
CA65=$(CC65_HOME)\\ca65.exe
CL65=$(CC65_HOME)\\cl65.exe

PRODUCT=YAIL
TARGET=atari
SRC_DIR=src
CFLAGS=-Or
LINKFLAGS=
#LINKFLAGS=--debug-info -Wl --dbgfile,"myapp.dbg"
# -D__SYSTEM_CHECK__=1
LIBRARIES=fujinet-lib/fujinet-atari-2.2.1.lib

.SUFFIXES:
.SUFFIXES: .c .s .o

all: $(PRODUCT).XEX

c_files: $(SRC_DIR)\*.c
    @echo Building $(**) to .s...
    @$(MAKE) -nologo /f Makefile.mak $(**:.c=.s)

s_files: $(SRC_DIR)\*.s
    @echo Building $(**) to .o...
    @$(MAKE) -nologo /f Makefile.mak $(**:.s=.o)

link_files: $(SRC_DIR)\*.o
    $(CL65) -t $(TARGET) $(CFLAGS) $(LINKFLAGS) -o $(PRODUCT).XEX --config $(SRC_DIR)\$(PRODUCT).$(TARGET)-xex.cfg --mapfile $(PRODUCT).map -Ln $(PRODUCT).lbl $(**) $(TARGET).lib $(LIBRARIES)

.s.o:
  $(CA65) -t $(TARGET) $<

.c.s:
  $(CC65) -t $(TARGET) $(CFLAGS) -I fujinet-lib/ $<

$(PRODUCT).XEX: c_files s_files link_files

clean: s_products c_products
  del $(PRODUCT).XEX $(PRODUCT).map

c_products: $(SRC_DIR)\*.c
    @echo Cleaning $(**:.c=.s)
    del $(**:.c=.s)

s_products: $(SRC_DIR)\*.s
    @echo Cleaning $(**:.s=.o)
    del $(**:.s=.o)

atr: $(PRODUCT).XEX
    copy $(PRODUCT).XEX atr
    dir2atr -D $(PRODUCT).ATR atr

