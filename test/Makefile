#ifeq ($(CROSS),)
#CROSS=arm-hisiv100nptl-linux-
#endif

#CROSS=
AR=$(CROSS)ar
CC=$(CROSS)gcc
CXX=$(CROSS)g++
STRIP=$(CROSS)strip

DATE=$(shell date "+%Y%m%d-%H%M")

CFLAGS += -Wall -Wno-unused-but-set-variable -Wno-deprecated -Wno-unused-variable -DBNO=\"$(DATE)\" -c -O2 -DHI_OS_LINUX -lpthread -Wno-unused-function -DHOST=\"$(HOST)\"
#CFLAGS+= -c -O2 -Wall -DHI_OS_LINUX -lpthread
EXFLAGS= 
LDFLAGS=

INC+=-I./inc -I/home/openipc/mywork/vos -I/home/openipc/lib/hisiv300/include
INC+=-I/home/openipc/mywork/vos/inc/hiredis/
DLIBS=-L./lib -lpthread -lstdc++ -lrt -lm
#if [ "$(HOST)" = "hisiv300" ];then
#echo "fuck it"
#DLIBS += -lNetLib
#fi
SLIBS=/home/openipc/lib/$(HOST)/slib/libvos.a /home/openipc/lib/$(HOST)/slib/libghttp.a
LIBS=$(DLIBS) $(SLIBS)

EXEC = vos_test
#OBJS = srs_librtmp.o uart2tcp.o sdkapi.o ttyuart.o
OBJS = vos_test.o

all:$(EXEC)

$(EXEC): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)
	$(STRIP) $(EXEC)
#	cp $@ ../$(HOST)

clean:
	rm -rf $(OBJS) $(EXEC) *.a

%.o: %.cpp
	$(CXX) -c $< $(INC)

%.o: %.c
	$(CC) -c $< ${CFLAGS} $(INC)

#%: %.cpp
#	$(CXX) -o $@ $< $(INC) $(LIBS)

#%: %.c
#	$(CC) -o $@ $< $(INC) $(LIBS)

