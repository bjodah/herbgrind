/*--------------------------------------------------------------------*/
/*--- Herbgrind: a valgrind tool for Herbie        instrument-op.c ---*/
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

#include "pub_tool_libcassert.h"
#include "pub_tool_machine.h"
#include "pub_tool_xarray.h"

#include "pub_tool_libcprint.h"

#include "instrument-op.h"
#include "instrument-storage.h"
#include "conversion.h"
#include "semantic-op.h"
#include "ownership.h"
#include "../runtime/shadowop/shadowop.h"
#include "../runtime/shadowop/conversions.h"
#include "../runtime/shadowop/exit-float-op.h"
#include "../runtime/value-shadowstate/exprs.h"
#include "../runtime/value-shadowstate/shadowval.h"
#include "../runtime/value-shadowstate/real.h"
#include "../helper/instrument-util.h"
#include "../helper/debug.h"
#include "../options.h"

void instrumentOp(IRSB* sbOut, IRTemp dest, IRExpr* expr,
                  Addr curAddr, Addr blockAddr){
  // Get the IROp value, the number of args, and the argument
  // expressions out of the structure based on whether it's a Unop, a
  // Binop, or a Triop.
  IROp op_code;
  int nargs;
  IRExpr* argExprs[4];
  switch(expr->tag){
  case Iex_Unop:
    op_code = expr->Iex.Unop.op;
    nargs = 1;
    argExprs[0] = expr->Iex.Unop.arg;
    break;
  case Iex_Binop:
    op_code = expr->Iex.Binop.op;
    switch(op_code){
    case Iop_Sqrt64Fx2:
    case Iop_SqrtF64:
    case Iop_SqrtF32:
    case Iop_RecpExpF64:
    case Iop_RecpExpF32:
    case Iop_SinF64:
    case Iop_CosF64:
    case Iop_TanF64:
    case Iop_2xm1F64:
      nargs = 1;
      argExprs[0] = expr->Iex.Binop.arg2;
      break;
    default:
      nargs = 2;
      argExprs[0] = expr->Iex.Binop.arg1;
      argExprs[1] = expr->Iex.Binop.arg2;
      break;
    }
    break;
  case Iex_Triop:
    op_code = expr->Iex.Triop.details->op;
    nargs = 2;
    argExprs[0] = expr->Iex.Triop.details->arg2;
    argExprs[1] = expr->Iex.Triop.details->arg3;
    break;
  case Iex_Qop:
    op_code = expr->Iex.Qop.details->op;
    nargs = 3;
    argExprs[0] = expr->Iex.Qop.details->arg2;
    argExprs[1] = expr->Iex.Qop.details->arg3;
    argExprs[2] = expr->Iex.Qop.details->arg4;
    break;
  default:
    tl_assert(0);
    return;
  }
  // If the op isn't a float op, dont shadow it.
  if (isSpecialOp(op_code)){
    handleSpecialOp(sbOut, op_code, argExprs, dest,
                    curAddr, blockAddr);
  } else if (isExitFloatOp(op_code) && mark_on_escape){
    handleExitFloatOp(sbOut, op_code, argExprs, dest,
                      curAddr, blockAddr);
  } else if (isFloatOp(op_code)){
    if (isConversionOp(op_code)){
      instrumentConversion(sbOut, op_code, argExprs, dest);
    } else {
      instrumentSemanticOp(sbOut, op_code, nargs, argExprs,
                           curAddr, blockAddr, dest);
    }
  } else {
    for(int i = 0; i < nargs; ++i){
      if (hasStaticShadow(argExprs[i])){
        ppIROp(op_code);
        VG_(printf)(" on ");
        ppIRExpr(argExprs[i]);
        VG_(printf)("\n");
        tl_assert(!hasStaticShadow(argExprs[i]));
      }
    }
    addStoreTempNonFloat(sbOut, dest);
  }
}
Bool isExitFloatOp(IROp op_code){
  switch(op_code){
  case Iop_CmpF32:
  case Iop_CmpEQ32Fx2:
  case Iop_CmpGT32Fx2:
  case Iop_CmpGE32Fx2:
  case Iop_CmpEQ32Fx4:
  case Iop_CmpLT32Fx4:
  case Iop_CmpLE32Fx4:
  case Iop_CmpUN32Fx4:
  case Iop_CmpEQ32F0x4:
  case Iop_CmpLT32F0x4:
  case Iop_CmpLE32F0x4:
  case Iop_CmpUN32F0x4:
  case Iop_CmpF64:
  case Iop_CmpEQ64Fx2:
  case Iop_CmpLT64Fx2:
  case Iop_CmpLE64Fx2:
  case Iop_CmpUN64Fx2:
  case Iop_CmpEQ64F0x2:
  case Iop_CmpLT64F0x2:
  case Iop_CmpLE64F0x2:
  case Iop_CmpUN64F0x2:
  case Iop_F64toI16S:
  case Iop_F64toI32S:
  case Iop_F64toI32U:
  case Iop_F64toI64S:
  case Iop_F64toI64U:
  case Iop_F32toI32S:
  case Iop_F32toI32U:
  case Iop_F32toI64S:
  case Iop_F32toI64U:
    return True;
  default:
    return False;
  }
}
void handleExitFloatOp(IRSB* sbOut, IROp op_code,
                       IRExpr** argExprs, IRTemp dest,
                       Addr curAddr, Addr blockAddr){
  switch(op_code){
  case Iop_CmpF64:
  case Iop_CmpF32:
  case Iop_CmpLT32F0x4:
  case Iop_CmpLT64F0x2:
    {
      ShadowCmpInfo* info =
        VG_(perm_malloc)(sizeof(ShadowCmpInfo),
                         vg_alignof(ShadowCmpInfo));
      info->op_addr = curAddr;
      info->op_code = op_code;
      info->precision = argPrecision(op_code);

      for(int i = 0; i < 2; ++i){
        addStoreC(sbOut, argExprs[i],
                  (uintptr_t)
                  (argPrecision(op_code) == Ft_Single ?
                   ((void*)computedArgs.argValuesF[i]) :
                   ((void*)computedArgs.argValues[i])));
        if (argExprs[i]->tag == Iex_RdTmp){
          info->argTemps[i] = argExprs[i]->Iex.RdTmp.tmp;
          cleanupAtEndOfBlock(sbOut, info->argTemps[i]);
        } else {
          info->argTemps[i] = -1;
        }
      }
      addStoreC(sbOut, IRExpr_RdTmp(dest), &computedResult);
      cleanupAtEndOfBlock(sbOut, dest);

      IRDirty* dirty =
        unsafeIRDirty_0_N(1, "checkCompare",
                          VG_(fnptr_to_fnentry)(checkCompare),
                          mkIRExprVec_1(mkU64((uintptr_t)
                                              info)));
      dirty->mFx = Ifx_Read;
      dirty->mAddr = mkU64((uintptr_t)&computedArgs);
      dirty->mSize =
        sizeof(computedArgs)
        + sizeof(computedResult)
        + sizeof(shadowTemps);
      addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));
    }
    break;
  case Iop_CmpEQ32Fx2:
  case Iop_CmpGT32Fx2:
  case Iop_CmpGE32Fx2:
  case Iop_CmpEQ32Fx4:
  case Iop_CmpLE32Fx4:
  case Iop_CmpUN32Fx4:
  case Iop_CmpLT32Fx4:
  case Iop_CmpEQ32F0x4:
  case Iop_CmpLE32F0x4:
  case Iop_CmpUN32F0x4:
    addPrintOp(op_code);
    addPrint3("\nArgs are {%f, %f} ",
              runUnop(sbOut, Iop_V128HIto64,
                      argExprs[0]),
              runUnop(sbOut, Iop_V128to64,
                      argExprs[0]));
    addPrint3("and {%f, %f} ",
              runUnop(sbOut, Iop_V128HIto64,
                      argExprs[1]),
              runUnop(sbOut, Iop_V128to64,
                      argExprs[1]));
    addPrint3("Result is {%lX, %lX}\n",
              runUnop(sbOut, Iop_V128HIto64, IRExpr_RdTmp(dest)),
              runUnop(sbOut, Iop_V128to64, IRExpr_RdTmp(dest)));
    addAssertNEQ(sbOut, "done.", mkU64(0), mkU64(0));
    break;
  case Iop_CmpLT64Fx2:
  case Iop_CmpEQ64Fx2:
  case Iop_CmpLE64Fx2:
  case Iop_CmpUN64Fx2:
  case Iop_CmpEQ64F0x2:
  case Iop_CmpLE64F0x2:
  case Iop_CmpUN64F0x2:
    tl_assert(0);
    break;
  case Iop_F64toI16S:
  case Iop_F64toI32S:
  case Iop_F64toI32U:
  case Iop_F64toI64S:
  case Iop_F64toI64U:
  case Iop_F32toI32S:
  case Iop_F32toI32U:
  case Iop_F32toI64S:
  case Iop_F32toI64U:
    {
      FloatType argPrecision;
      switch(op_code){
      case Iop_F64toI16S:
      case Iop_F64toI32S:
      case Iop_F64toI32U:
      case Iop_F64toI64S:
      case Iop_F64toI64U:
        argPrecision = Ft_Double;
        break;
      case Iop_F32toI32S:
      case Iop_F32toI32U:
      case Iop_F32toI64S:
      case Iop_F32toI64U:
      default:
        argPrecision = Ft_Single;
        break;
      }
      int argTemp;
      addStoreC(sbOut, argExprs[1],
                (uintptr_t)
                (argPrecision == Ft_Single ?
                 ((void*)computedArgs.argValuesF[0]) :
                 ((void*)computedArgs.argValues[0])));
      if (argExprs[1]->tag == Iex_RdTmp){
        argTemp = argExprs[1]->Iex.RdTmp.tmp;
        cleanupAtEndOfBlock(sbOut, argTemp);
      } else {
        argTemp = -1;
      }
      addStoreC(sbOut, IRExpr_RdTmp(dest), &computedResult);
      cleanupAtEndOfBlock(sbOut, dest);

      IRDirty* dirty =
        unsafeIRDirty_0_N(2, "checkCompare",
                          VG_(fnptr_to_fnentry)(checkConvert),
                          mkIRExprVec_3(mkU64(argPrecision),
                                        mkU64(argTemp),
                                        mkU64(curAddr)));
      dirty->mFx = Ifx_Read;
      dirty->mAddr = mkU64((uintptr_t)&computedArgs);
      dirty->mSize =
        sizeof(computedArgs)
        + sizeof(computedResult)
        + sizeof(shadowTemps);
      addStmtToIRSB(sbOut, IRStmt_Dirty(dirty));
    }
    break;
  default:
    return;
  }
}
Bool isSpecialOp(IROp op_code){
  switch(op_code){
  case Iop_I32StoF64:
  case Iop_I64StoF64:
  case Iop_XorV128:
  case Iop_AndV128:
  case Iop_OrV128:
  case Iop_NotV128:
    return True;
  default:
    return False;
  }
}

void handleSpecialOp(IRSB* sbOut, IROp op_code,
                     IRExpr** argExprs, IRTemp dest,
                     Addr curAddr, Addr block_addr){
  switch(op_code){
  case Iop_I32StoF64:
  case Iop_I64StoF64:
  case Iop_AndV128:
  case Iop_OrV128:
  case Iop_NotV128:
    addStoreTempUnshadowed(sbOut, dest);
    break;
  case Iop_XorV128:
    instrumentPossibleNegate(sbOut, argExprs, dest, curAddr, block_addr);
    break;
  default:
    tl_assert(0);
    break;
  }
}

Bool isFloatOp(IROp op_code){
  switch(op_code){
  case Iop_32UtoV128:
  case Iop_64UtoV128:
  case Iop_RecipEst32Fx4:
  case Iop_RSqrtEst32Fx4:
  case Iop_Abs32Fx4:
  case Iop_Neg32Fx4:
  case Iop_ZeroHI96ofV128:
  case Iop_ZeroHI64ofV128:
  case Iop_V128to32:
  case Iop_V128to64:
  case Iop_V128HIto64:
  case Iop_SetV128lo32:
  case Iop_SetV128lo64:
  case Iop_RecipEst64Fx2:
  case Iop_RSqrtEst64Fx2:
  case Iop_Abs64Fx2:
  case Iop_Neg64Fx2:
  case Iop_RecipEst32F0x4:
  case Iop_Sqrt32F0x4:
  case Iop_RSqrtEst32F0x4:
  case Iop_RSqrtEst32Fx2:
  case Iop_RecipEst32Fx2:
  case Iop_RoundF64toF32:
  case Iop_TruncF64asF32:
  case Iop_RSqrtEst5GoodF64:
  case Iop_RoundF64toF64_NEAREST:
  case Iop_RoundF64toF64_NegINF:
  case Iop_RoundF64toF64_PosINF:
  case Iop_RoundF64toF64_ZERO:
  case Iop_F128HItoF64:
  case Iop_F128LOtoF64:
  case Iop_F32toF64:
  case Iop_NegF32:
  case Iop_AbsF32:
  case Iop_NegF64:
  case Iop_AbsF64:
  case Iop_Sqrt64F0x2:
    // Binary Ops
  case Iop_RecipStep32Fx4:
  case Iop_RSqrtStep32Fx4:
  case Iop_Add32Fx2:
  case Iop_Sub32Fx2:
  case Iop_Sqrt64Fx2:
  case Iop_Add32F0x4:
  case Iop_Sub32F0x4:
  case Iop_Mul32F0x4:
  case Iop_Div32F0x4:
  case Iop_RecipStep32Fx2:
  case Iop_RSqrtStep32Fx2:
  case Iop_RecipStep64Fx2:
  case Iop_RSqrtStep64Fx2:
  case Iop_Neg32Fx2:
  case Iop_Abs32Fx2:
  case Iop_RecpExpF64:
  case Iop_RecpExpF32:
  case Iop_RoundF64toInt:
  case Iop_RoundF32toInt:
  case Iop_64HLtoV128:
  case Iop_F64HLtoF128:
  case Iop_F64toF32:
  case Iop_SinF64:
  case Iop_CosF64:
  case Iop_TanF64:
  case Iop_2xm1F64:
  case Iop_SqrtF64:
  case Iop_SqrtF32:
  case Iop_Mul64F0x2:
  case Iop_Div64F0x2:
  /* case Iop_XorV128: */
  case Iop_Sub64F0x2:
  case Iop_Add64F0x2:
  case Iop_Max64F0x2:
  case Iop_Max64Fx2:
  case Iop_Max32F0x4:
  case Iop_Max32Fx4:
  case Iop_Max32Fx2:
  case Iop_Min64F0x2:
  case Iop_Min64Fx2:
  case Iop_Min32F0x4:
  case Iop_Min32Fx4:
  case Iop_Min32Fx2:
    // Ternary Ops
  case Iop_Add32Fx8:
  case Iop_Sub32Fx8:
  case Iop_Mul32Fx8:
  case Iop_Div32Fx8:
  case Iop_Add64Fx4:
  case Iop_Sub64Fx4:
  case Iop_Mul64Fx4:
  case Iop_Div64Fx4:
  case Iop_Add32Fx4:
  case Iop_Sub32Fx4:
  case Iop_Mul32Fx4:
  case Iop_Div32Fx4:
  case Iop_Add64Fx2:
  case Iop_Sub64Fx2:
  case Iop_Mul64Fx2:
  case Iop_Div64Fx2:
  case Iop_AtanF64:
  case Iop_Yl2xF64:
  case Iop_Yl2xp1F64:
  case Iop_ScaleF64:
  case Iop_AddF128:
  case Iop_SubF128:
  case Iop_MulF128:
  case Iop_DivF128:
  case Iop_AddF64:
  case Iop_SubF64:
  case Iop_MulF64:
  case Iop_DivF64:
  case Iop_AddF32:
  case Iop_SubF32:
  case Iop_MulF32:
  case Iop_DivF32:
  case Iop_AddF64r32:
  case Iop_SubF64r32:
  case Iop_MulF64r32:
  case Iop_DivF64r32:
    // Quadnary ops
  case Iop_MAddF32:
  case Iop_MSubF32:
  case Iop_MAddF64:
  case Iop_MSubF64:
  case Iop_MAddF64r32:
  case Iop_MSubF64r32:
    return True;
  default:
    return False;
  }
}
