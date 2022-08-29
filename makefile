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
 DRATS.o FreeDATA.o base64.o

# Configuration:

CC = gcc
		                       
all: CFLAGS = -DLINBPQ -MMD -g -fcommon
all: linbpq


noi2c: CFLAGS = -DLINBPQ -MMD -DNOI2C -g
noi2c: linbpq

	
linbpq: $(OBJS)
	gcc $(OBJS) -Xlinker -Map=output.map -l:libminiupnpc.a -lcrypto -lrt -lm -lz -lpthread -lconfig -lpcap -o linbpq
	sudo setcap "CAP_NET_ADMIN=ep CAP_NET_RAW=ep CAP_NET_BIND_SERVICE=ep" linbpq		

-include *.d

clean :
	rm *.d
	rm linbpq $(OBJS)

