//
//	Standard __except handler to dump stack and code around eip
//

__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)
{
	unsigned __int32 SPPtr = 0;
	unsigned __int32 SPVal = 0;
	unsigned __int32 eip = 0;
	unsigned __int32 rev = 0;
	int i;

	DWORD Stack[16];
	DWORD CodeDump[16];

#ifndef _WIN64

	eip = exinfo.ContextRecord->Eip;	
	SPPtr = exinfo.ContextRecord->Esp;	

	__asm
	{
		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

		lea edi,CodeDump
		mov esi,eip
		mov ecx,64
		rep movsb
	}



	Debugprintf("BPQ32 *** Program Error %x at %x in %s",
		exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress, EXCEPTMSG);

	Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
		exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,
		exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi, SPVal);
		
#endif

	Debugprintf("Stack:");

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

	Debugprintf("Code:");

	for (i = 0; i < 16; i++)
	{
		rev = (CodeDump[i] & 0xff) << 24;
		rev |= (CodeDump[i] & 0xff00) << 8;
		rev |= (CodeDump[i] & 0xff0000) >> 8;
		rev |= (CodeDump[i] & 0xff000000) >> 24;

		CodeDump[i] = rev;
	}

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		eip, CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		eip+32, CodeDump[8], CodeDump[9], CodeDump[10], CodeDump[11], CodeDump[12], CodeDump[13], CodeDump[14], CodeDump[15]);

	WriteMiniDump();

	// Note - no closing } so additional code may be run in the __except block

#ifdef MDIKERNEL
	if (CloseOnError == 1)
		CloseAllNeeded = 1;
#endif

#undef EXCEPTMSG
