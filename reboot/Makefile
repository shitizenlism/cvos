AR=$(CROSS)ar
CC=$(CROSS)gcc
CXX=$(CROSS)g++
STRIP=$(CROSS)strip

ARFLAGS=-rc
LDFLAGS=-shared -Wl -s
CFLAGS=-w -fPIC
DATE=$(shell date "+%Y%m%d-%H%M")
MACRO+= -D BUILDNO=\"$(DATE)\"

OBJ+=libreboot.a reboot

include ./common.mk
#INC=-I ../libcfgfile

DLIB=-L ./lib
SLIB=

LIBOBJS= reboot.o

libreboot.so: $(LIBOBJS)
	$(CC) $(INC) $(LDFLAGS) -o $@ $^ $(SLIB) $(DLIB)
	$(STRIP) $@
#	cp -av $@ ./lib
	
libreboot.a: $(LIBOBJS)
	$(AR) $(ARFLAGS) -o $@ $^

reboot: $(LIBOBJS)
	rm -f $(LIBOBJS)
	$(CC) -o $@ reboot.c $(INC) libreboot.a $(DLIB) -DSMAIN
	$(STRIP) $@
	#cp -av $@ ../../../bin/$(HOST)
	@echo "$(DATE)">>build.log
clean: 
	rm -f $(OBJ)
