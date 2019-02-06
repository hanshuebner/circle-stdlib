#
# Makefile
#

CIRCLEHOME = ../../libs/circle
NEWLIBDIR = ../../install/arm-none-circle

OBJS	= main.o rpmv0.o roms.o kernel.o vectors.o

include $(CIRCLEHOME)/Rules.mk

main.o: kernel.h

sound.h: sound.h.gz
	@echo "  UNZIP $@"
	@gunzip -c sound.h.gz > sound.h

CFLAGS += -I "$(NEWLIBDIR)/include" -I $(STDDEF_INCPATH) -I ../../include
LIBS := "$(NEWLIBDIR)/lib/libm.a" "$(NEWLIBDIR)/lib/libc.a" "$(NEWLIBDIR)/lib/libcirclenewlib.a" \
 	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
  	$(CIRCLEHOME)/lib/usb/libusb.a \
 	$(CIRCLEHOME)/lib/input/libinput.a \
 	$(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
 	$(CIRCLEHOME)/lib/fs/libfs.a \
  	$(CIRCLEHOME)/lib/net/libnet.a \
  	$(CIRCLEHOME)/lib/sched/libsched.a \
  	$(CIRCLEHOME)/lib/libcircle.a
