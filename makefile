#	LinBPQ Makefile

#	To exclude i2c support run make noi2c

OBJS = pngwtran.o pngrtran.o pngset.o pngrio.o pngwio.o pngtrans.o pngrutil.o pngwutil.o\
 pngread.o pngwrite.o png.o pngerror.o pngget.o pngmem.o APRSIconData.o AISCommon.o\
 upnp.o APRSStdPages.o HSMODEM.o WinRPR.o KISSHF.o TNCEmulators.o bpqhdlc.o SerialPort.o\
 adif.o WebMail.o utf8Routines.o VARA.o LzFind.o Alloc.o LzmaDec.o LzmaEnc.o LzmaLib.o \
 Multicast.o ARDOP.o IPCode.o FLDigi.o linether.o CMSAuth.o APRSCode.o BPQtoAGW.o KAMPactor.o\
 AEAPactor.o HALDriver.o MULTIPSK.o BBSHTMLConfig.o ChatHTMLConfig.o BBSUtilities.o bpqaxip.o\
 BPQINP3.o BPQNRR.o cMain.o Cmd.o CommonCode.o HTMLCommonCode.o compatbits.o config.o datadefs.o \
 FBBRoutines.o HFCommon.o Housekeeping.o HTTPcode.o kiss.o L2Code.o L3Code.o L4Code.o lzhuf32.o \
 MailCommands.o MailDataDefs.o LinBPQ.o MailRouting.o MailTCP.o MBLRoutines.o md5.o Moncode.o \
 NNTPRoutines.o RigControl.o TelnetV6.o WINMOR.o TNCCode.o UZ7HODrv.o WPRoutines.o \
 SCSTrackeMulti.o SCSPactor.o SCSTracker.o HanksRT.o  UIRoutines.o AGWAPI.o AGWMoncode.o \
 DRATS.o FreeDATA.o base64.o Events.o nodeapi.o mailapi.o mqtt.o RHP.o

# Configuration:

#Default to Linux
	CC = gcc    
	LDFLAGS = -Xlinker -Map=output.map -lrt 

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)	
all: LIBS = -lpaho-mqtt3a -ljansson -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                       
all: linbpq

#other OS

OS_NAME = $(shell uname -s)
ifeq ($(OS_NAME),NetBSD)
    CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/pkg/include
    LDFLAGS = -Xlinker -Map=output.map -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lrt -lutil -lexecinfo

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)
all: LIBS = -lminiupnpc -lm -lz -lpthread -lconfig -lpcap
all: linbpq
endif

ifeq ($(OS_NAME),OpenBSD)
    CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/local/include
    LDFLAGS = -Xlinker -Map=output.map -L/usr/local/lib -liconv -lutil -lexecinfo

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)
all: LIBS = -lminiupnpc -lm -lz -lpthread -lconfig -lpcap
all: linbpq
endif

ifeq ($(OS_NAME),DragonFly)
    CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/local/include
    LDFLAGS = -Xlinker -Map=output.map -L/usr/local/lib -lrt -liconv -lutil -lexecinfo

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)
all: LIBS =  -lminiupnpc -lm -lz -lpthread -lconfig -lpcap
all: linbpq
endif

ifeq ($(OS_NAME),FreeBSD)
    CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/local/include
    LDFLAGS = -Xlinker -Map=output.map -L/usr/local/lib -lrt -liconv -lutil -lexecinfo

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)
all: LIBS =  -lminiupnpc -lm -lz -lpthread -lconfig -lpcap
all: linbpq
endif

ifeq ($(OS_NAME),Darwin)
	CC = gcc	                       
	EXTRA_CFLAGS = -DMACBPQ -DNOMQTT 
	LDFLAGS = -liconv

all: CFLAGS = -DLINBPQ  -MMD -g -fcommon -fasynchronous-unwind-tables $(EXTRA_CFLAGS)	
all: LIBS = -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                     
all: linbpq
endif

$(info OS_NAME is $(OS_NAME))



nomqtt: CFLAGS = -DLINBPQ -MMD -fcommon -g  -rdynamic -DNOMQTT -fasynchronous-unwind-tables
nomqtt: LIBS = -lminiupnpc -lm -lz -lpthread -lconfig -lpcap   
nomqtt: linbpq

noi2c: CFLAGS = -DLINBPQ -MMD -DNOI2C -g  -rdynamic -fcommon -fasynchronous-unwind-tables
noi2c: LIBS = -lpaho-mqtt3a -ljansson -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                        
noi2c: linbpq


linbpq: $(OBJS)
	cc $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o linbpq
ifeq ($(OS_NAME),Linux)
	sudo setcap "CAP_NET_ADMIN=ep CAP_NET_RAW=ep CAP_NET_BIND_SERVICE=ep" linbpq		
endif

-include *.d

clean :
	rm -f *.d
	rm -f linbpq $(OBJS)

