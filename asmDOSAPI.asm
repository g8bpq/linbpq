IF 0

Copyright 2001-2015 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses

ENDIF

IFNDEF BPQ64

	PAGE    56,132
; 

.386
;
;  SEGMENT definitions and order
;


;*	32 Bit code
_TEXT		SEGMENT DWORD USE32 PUBLIC 'CODE'
_TEXT		ENDS



;*	Contains 32 Bit data
_BPQDATA		SEGMENT DWORD PUBLIC 'DATA'
_BPQDATA		ENDS


	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>

_BPQDATA	SEGMENT

	extern _APISemaphore:DWORD

ApiEAX DD 0;
ApiEBX DD 0;
ApiECX DD 0;
ApiEDX DD 0;
ApiESI DD 0;
ApiEDI DD 0;

_BPQDATA	ENDS

_TEXT	SEGMENT
;
	EXTRN	_CHOSTAPI:NEAR
MARKER	DB	'G8BPQ'			; MUST BE JUST BEFORE INT 7F ENTRY
	DB	4 ; MAJORVERSION
	DB	9 ; MINORVERSION


	PUBLIC	_BPQHOSTAPI
_BPQHOSTAPI:
;
;	SPECIAL INTERFACE, MAINLY FOR EXTERNAL HOST MODE SUPPORT PROGS
;
	extrn	_GetSemaphore:near
	extrn	_FreeSemaphore:near
	extrn	_Check_Timer:near


	pushad
	call	_Check_Timer
	push	offset _APISemaphore
	call	_GetSemaphore
	add		esp, 4
	popad
	
;	Params are 16 bits

	movzx	eax,ax
	movzx	ebx,bx
	movzx	ecx,cx
	movzx	edx,dx
	
	mov	ApiEAX, eax
	mov	ApiEBX, ebx
	mov	ApiECX, ecx
	mov	ApiEDX, edx
	mov	ApiESI, esi	
	mov	ApiEDI, edi

	lea		eax,ApiEDI
	push	eax
	lea		eax,ApiESI
	push	eax
	lea		eax,ApiEDX
	push	eax
	lea		eax,ApiECX
	push	eax
	lea		eax,ApiEBX
	push	eax
	lea		eax,ApiEAX
	push	eax
	
	call	_CHOSTAPI
	add		esp, 24
	
	mov	eax,ApiEAX
	mov	ebx,ApiEBX
	mov	ecx,ApiECX
	mov	edx,ApiEDX
	mov	esi,ApiESI	
	mov	esi,ApiEDI

	
	pushad
	push	offset _APISemaphore
	call	_FreeSemaphore
	add		esp, 4
	popad

	ret

_TEXT	ENDS
ENDIF

END
	
