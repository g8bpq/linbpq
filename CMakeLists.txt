cmake_minimum_required(VERSION 3.18)

project(LinBPQ)
 
# Our Project

set(CMAKE_C_FLAGS "-DLINBPQ -g -fcommon -fasynchronous-unwind-tables")

set(SOURCE_FILES
pngwtran.c pngrtran.c pngset.c pngrio.c pngwio.c pngtrans.c pngrutil.c pngwutil.c
 pngread.c pngwrite.c png.c pngerror.c pngget.c pngmem.c APRSIconData.c AISCommon.c
 upnp.c APRSStdPages.c HSMODEM.c WinRPR.c KISSHF.c TNCEmulators.c bpqhdlc.c SerialPort.c
 adif.c WebMail.c utf8Routines.c VARA.c LzFind.c Alloc.c LzmaDec.c LzmaEnc.c LzmaLib.c 
 Multicast.c ARDOP.c IPCode.c FLDigi.c linether.c CMSAuth.c APRSCode.c BPQtoAGW.c KAMPactor.c
 AEAPactor.c HALDriver.c MULTIPSK.c BBSHTMLConfig.c ChatHTMLConfig.c BBSUtilities.c bpqaxip.c
 BPQINP3.c BPQNRR.c cMain.c Cmd.c CommonCode.c HTMLCommonCode.c compatbits.c config.c datadefs.c 
 FBBRoutines.c HFCommon.c Housekeeping.c HTTPcode.c kiss.c L2Code.c L3Code.c L4Code.c lzhuf32.c 
 MailCommands.c MailDataDefs.c LinBPQ.c MailRouting.c MailTCP.c MBLRoutines.c md5.c Moncode.c 
 NNTPRoutines.c RigControl.c TelnetV6.c WINMOR.c TNCCode.c UZ7HODrv.c WPRoutines.c 
 SCSTrackeMulti.c SCSPactor.c SCSTracker.c HanksRT.c  UIRoutines.c AGWAPI.c AGWMoncode.c 
 DRATS.c FreeDATA.c base64.c Events.c nodeapi.c mailapi.c mqtt.c RHP.c NETROMTCP.c)
 
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

