/*--------------------------------------------------------------------*/
/*--- Herbgrind: a valgrind tool for Herbie                 real.h ---*/
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

#ifndef _REAL_H
#define _REAL_H

#include "../../options.h"
#include "../../helper/runtime-util.h"

#ifdef USE_MPFR
#include "mpfr.h"
#else
#include "gmp.h"
#endif

#include "pub_tool_basics.h"

typedef struct _RealStruct{
  #ifdef USE_MPFR
  mpfr_t mpfr_val;
  #else
  mpf_t mpf_val;
  #endif
} *Real;

Real mkReal(void);
void setReal(Real r, double bytes);

double getDouble(Real real);
int isNaN(Real real);
int realCompare(Real real1, Real real2);

void freeReal(Real real);
void copyReal(Real src, Real dest);
void printBBufReal(BBuf* buf, Real real);
void printBBufFloatAsReal(BBuf* buf, double val);
void pFloat(BBuf* buf, double val);
void printReal(Real real);

inline void setReal_fast(Real r, double bytes);

__attribute__((always_inline))
inline
void setReal_fast(Real r, double bytes){
  if (no_reals) return;
  #ifdef USE_MPFR
  mpfr_set_d(r->mpfr_val, bytes, MPFR_RNDN);
  #else
  mpf_set_d(r->mpf_val, bytes);
  #endif
}

#endif
