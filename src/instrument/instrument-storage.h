/*--------------------------------------------------------------------*/
/*--- Herbgrind: a valgrind tool for Herbie   instrument-storage.h ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Herbgrind, a valgrind tool for diagnosing
   floating point accuracy problems in binary programs and extracting
   problematic expressions.

   Copyright (C) 2016-2017 Alex Sanchez-Stern

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/
#ifndef _INSTRUMENT_STORAGE_H
#define _INSTRUMENT_STORAGE_H

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

#include "floattypes.h"
#include "../helper/stack.h"

void initInstrumentationState(void);
void instrumentRdTmp(IRSB* sbOut, IRTemp dest, IRTemp src);
void instrumentWriteConst(IRSB* sbOut, IRTemp dest,
                          IRConst* con);
void instrumentITE(IRSB* sbOut, IRTemp dest,
                   IRExpr* cond,
                   IRExpr* trueExpr, IRExpr* falseExpr);
void instrumentPut(IRSB* sbOut, Int tsDest, IRExpr* data,
                   int instrIdx);
void instrumentPutI(IRSB* sbOut,
                    IRExpr* varOffset, Int constOffset,
                    Int arrayBase, Int numElems, IRType elemType,
                    IRExpr* data,
                    int instrIdx);
void instrumentGet(IRSB* sbOut, IRTemp dest,
                   Int tsSrc, IRType type,
                   int instrIdx);
void instrumentGetI(IRSB* sbOut, IRTemp dest,
                    IRExpr* varOffset, Int constOffset,
                    Int arrayBase, Int numElems, IRType elemType,
                    int instrIdx);
void instrumentLoad(IRSB* sbOut, IRTemp dest,
                    IRExpr* addr, IRType type);
void instrumentLoadSmallButSlow(IRSB* sbOut, IRTemp dest,
                                IRExpr* addr, IRType type);
#define LOADG_FALLBACK_THRESHOLD 150
#define LOAD_FALLBACK_THRESHOLD 215
void instrumentLoadG(IRSB* sbOut, IRTemp dest,
                     IRExpr* altValue, IRExpr* guard,
                     IRExpr* addr, IRLoadGOp conversion);
void instrumentLoadGSmallButSlow(IRSB* sbOut, IRTemp dest,
                                 IRExpr* altValue, IRExpr* guard,
                                 IRExpr* addr, IRLoadGOp conversion);
void instrumentStore(IRSB* sbOut, IRExpr* addr,
                     IRExpr* data);
void instrumentStoreG(IRSB* sbOut, IRExpr* addr,
                      IRExpr* guard, IRExpr* data);
void instrumentCAS(IRSB* sbOut,
                   IRCAS* details);
void finishInstrumentingBlock(IRSB* sbOut);
void addBlockCleanupG(IRSB* sbOut, IRExpr* guard);

IRExpr* runMkShadowTempValues(IRSB* sbOut, FloatBlocks num_blocks,
                              IRExpr** values);
IRExpr* runMkShadowTempValuesG(IRSB* sbOut,
                               IRExpr* guard, IRExpr* guard32,
                               FloatBlocks num_blocks,
                               IRExpr** values);
IRExpr* runMkShadowVal(IRSB* sbOut, ValueType type, IRExpr* valExpr);
IRExpr* runMkShadowValG(IRSB* sbOut, IRExpr* guard,
                        ValueType type, IRExpr* valExpr);
IRExpr* runMakeInputG(IRSB* sbOut, IRExpr* guard, IRExpr* argExpr,
                      ValueType type);
IRExpr* runMakeInput(IRSB* sbOut, IRExpr* argExpr, ValueType type);

IRExpr* runGetTSVal(IRSB* sbOut, Int tsSrc, int instrIdx);
IRExpr* runGetTSValDynamic(IRSB* sbOut, IRExpr* tsSrc);
void addSetTSValNonNull(IRSB* sbOut, Int tsDest,
                        IRExpr* newVal,
                        int instrIdx);
void addSetTSValNonFloat(IRSB* sbOut, Int tsDest,
                         int instrIdx);
void addSetTSValUnshadowed(IRSB* sbOut, Int tsDest,
                           int instrIdx);
void addSetTSValUnknown(IRSB* sbOut, Int tsDest, IRExpr* newVal,
                        int instrIdx);
void addSetTSVal(IRSB* sbOut, Int tsDest, IRExpr* newVal, int instrIdx);
void addSetTSValDynamic(IRSB* sbOut, IRExpr* tsDest, IRExpr* newVal, int instrIdx);

IRExpr* runLoadTemp(IRSB* sbOut, int idx);
void addStoreTemp(IRSB* sbOut, IRExpr* shadow_temp,
                  int idx);
void addStoreTempG(IRSB* sbOut, IRExpr* guard,
                   IRExpr* shadow_temp,
                   int idx);
void addStoreTempNonFloat(IRSB* sbOut, int idx);
void addStoreTempUnknown(IRSB* sbOut, IRExpr* shadow_temp_maybe, int idx);
void addStoreTempCopy(IRSB* sbOut, IRExpr* original, IRTemp dest);

IRExpr* getBucketAddr(IRSB* sbOut, IRExpr* memAddr);
typedef struct {
  IRExpr* entry;
  IRExpr* stillSearching32;
} QuickBucketResult;
QuickBucketResult quickGetBucket(IRSB* sbOut, IRExpr* memAddr);
QuickBucketResult quickGetBucketG(IRSB* sbOut, IRExpr* guard,
                                  IRExpr* memAddr);
IRExpr* runGetMemUnknown(IRSB* sbOut, FloatBlocks size, IRExpr* memSrc);
IRExpr* runGetMemUnknownG(IRSB* sbOut, IRExpr* guard,
                          FloatBlocks size, IRExpr* memSrc);
IRExpr* runGetMem(IRSB* sbOut, FloatBlocks size, IRExpr* memSrc);
IRExpr* runGetMemG(IRSB* sbOut, IRExpr* guard, FloatBlocks size, IRExpr* memSrc);
void addSetMemNonNull(IRSB* sbOut, FloatBlocks size,
                      IRExpr* memDest, IRExpr* newTemp);
void addSetMemG(IRSB* sbOut, IRExpr* guard, FloatBlocks size,
                IRExpr* memDest, IRExpr* newTemp);
void addClearMem(IRSB* sbOut, FloatBlocks size, IRExpr* memDest);
void addClearMemG(IRSB* sbOut, IRExpr* guard, FloatBlocks size, IRExpr* memDest);
void addSetMemUnknown(IRSB* sbOut, FloatBlocks size,
                      IRExpr* memDest, IRExpr* st);
void addSetMemUnknownG(IRSB* sbOut, IRExpr* guard, FloatBlocks size,
                       IRExpr* memDest, IRExpr* st);

IRExpr* toDoubleBytes(IRSB* sbOut, IRExpr* floatExpr);
IRExpr* mkArrayLookupExpr(IRSB* sbOut,
                          Int base, IRExpr* idx,
                          Int bias, Int len,
                          IRType elemSize);
#endif
