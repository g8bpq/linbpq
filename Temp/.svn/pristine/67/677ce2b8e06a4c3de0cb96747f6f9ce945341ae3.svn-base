/* LzFind.h -- Match finder for LZ algorithms
 2008-10-04 : Igor Pavlov : Public domain */

#ifndef __LZFIND_H
#define __LZFIND_H

#include "types.h"

typedef LZ_UInt32 CLzRef;

typedef struct _CMatchFinder
{
	Byte *buffer;
	LZ_UInt32 pos;
	LZ_UInt32 posLimit;
	LZ_UInt32 streamPos;
	LZ_UInt32 lenLimit;

	LZ_UInt32 cyclicBufferPos;
	LZ_UInt32 cyclicBufferSize; /* it must be = (historySize + 1) */

	LZ_UInt32 matchMaxLen;
	CLzRef *hash;
	CLzRef *son;
	LZ_UInt32 hashMask;
	LZ_UInt32 cutValue;

	Byte *bufferBase;
	ISeqInStream *stream;
	int streamEndWasReached;

	LZ_UInt32 blockSize;
	LZ_UInt32 keepSizeBefore;
	LZ_UInt32 keepSizeAfter;

	LZ_UInt32 numHashBytes;
	int directInput;
	int btMode;
	/* int skipModeBits; */
	int bigHash;
	LZ_UInt32 historySize;
	LZ_UInt32 fixedHashSize;
	LZ_UInt32 hashSizeSum;
	LZ_UInt32 numSons;
	SRes result;
	LZ_UInt32 crc[256];
} CMatchFinder;

#define Inline_MatchFinder_GetPointerToCurrentPos(p) ((p)->buffer)
#define Inline_MatchFinder_GetIndexByte(p, index) ((p)->buffer[(LZ_Int32)(index)])

#define Inline_MatchFinder_GetNumAvailableBytes(p) ((p)->streamPos - (p)->pos)

int MatchFinder_NeedMove(CMatchFinder *p);
Byte *MatchFinder_GetPointerToCurrentPos(CMatchFinder *p);
void MatchFinder_MoveBlock(CMatchFinder *p);
void MatchFinder_ReadIfRequired(CMatchFinder *p);

void MatchFinder_Construct(CMatchFinder *p);

/* Conditions:
 historySize <= 3 GB
 keepAddBufferBefore + matchMaxLen + keepAddBufferAfter < 511MB
 */
int MatchFinder_Create(CMatchFinder *p, LZ_UInt32 historySize,
					   LZ_UInt32 keepAddBufferBefore, LZ_UInt32 matchMaxLen, LZ_UInt32 keepAddBufferAfter,
					   ISzAlloc *alloc);
void MatchFinder_Free(CMatchFinder *p, ISzAlloc *alloc);
void MatchFinder_Normalize3(LZ_UInt32 subValue, CLzRef *items, LZ_UInt32 numItems);
void MatchFinder_ReduceOffsets(CMatchFinder *p, LZ_UInt32 subValue);

LZ_UInt32 * GetMatchesSpec1(LZ_UInt32 lenLimit, LZ_UInt32 curMatch, LZ_UInt32 pos, const Byte *buffer, CLzRef *son,
							LZ_UInt32 _cyclicBufferPos, LZ_UInt32 _cyclicBufferSize, LZ_UInt32 _cutValue,
							LZ_UInt32 *distances, LZ_UInt32 maxLen);

/*
 Conditions:
 Mf_GetNumAvailableBytes_Func must be called before each Mf_GetMatchLen_Func.
 Mf_GetPointerToCurrentPos_Func's result must be used only before any other function
 */

typedef void (*Mf_Init_Func)(void *object);
typedef Byte (*Mf_GetIndexByte_Func)(void *object, LZ_Int32 index);
typedef LZ_UInt32 (*Mf_GetNumAvailableBytes_Func)(void *object);
typedef const Byte * (*Mf_GetPointerToCurrentPos_Func)(void *object);
typedef LZ_UInt32 (*Mf_GetMatches_Func)(void *object, LZ_UInt32 *distances);
typedef void (*Mf_Skip_Func)(void *object, LZ_UInt32);

typedef struct _IMatchFinder
{
	Mf_Init_Func Init;
	Mf_GetIndexByte_Func GetIndexByte;
	Mf_GetNumAvailableBytes_Func GetNumAvailableBytes;
	Mf_GetPointerToCurrentPos_Func GetPointerToCurrentPos;
	Mf_GetMatches_Func GetMatches;
	Mf_Skip_Func Skip;
} IMatchFinder;

void MatchFinder_CreateVTable(CMatchFinder *p, IMatchFinder *vTable);

void MatchFinder_Init(CMatchFinder *p);
LZ_UInt32 Bt3Zip_MatchFinder_GetMatches(CMatchFinder *p, LZ_UInt32 *distances);
LZ_UInt32 Hc3Zip_MatchFinder_GetMatches(CMatchFinder *p, LZ_UInt32 *distances);
void Bt3Zip_MatchFinder_Skip(CMatchFinder *p, LZ_UInt32 num);
void Hc3Zip_MatchFinder_Skip(CMatchFinder *p, LZ_UInt32 num);

#endif
