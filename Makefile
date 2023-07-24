AR=$(CROSS)ar
CC=$(CROSS)gcc
CXX=$(CROSS)g++
STRIP=$(CROSS)strip

LDFLAGS=-shared -s -fPIC
CFLAGS=-fPIC -Wno-unused-but-set-variable -Wno-redundant-decls -Wshadow -Wpointer-arith -Wunreachable-code -Wno-format -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
ARFLAGS=-rc

INCLUDE+= -I./inc/ghttp/ -I./inc/hiredis
LIBPATH= -Llib/ lib/libghttp.a

LIB_USR=libvos.so
SLIB_USR=libvos.a

OBJ_USR=vos.o
SOBJ_USR=vos.o

DATE=$(shell date "+%Y%m%d-%H%M")
MACRO+=-DBNO=\"$(DATE)\"

all: $(SLIB_USR)

#-----------------------generate shared lib----------------------
$(LIB_USR): $(OBJ_USR)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_USR) $(MACRO) $(LIBPATH)
	$(STRIP) $(LIB_USR)
#	cp $@ ../../lib/$(HOST)
#	rm $(OBJ_USR)
$(OBJ_USR):%.o: %.c
	$(CC) $(INCLUDE) $(CFLAGS) $< -c $(MACRO)
	
#-----------------------generate static lib-----------------------------
slib:	$(SLIB_USR)
$(SLIB_USR): $(SOBJ_USR)
	$(AR) $(ARFLAGS) -o $@ $(SOBJ_USR)
	mv $@ lib
#	rm $(OBJ_USR)
#$(SOBJ_USR):%.o: %.c
#	$(CC) $(INCLUDE) $< -c	$(MACRO)

#----------------------------------------clean all-------------------------
clean:
	-$(RM) *.so *.o *.a

