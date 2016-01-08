#include "hg_evaluate.h"

void evaluateOpError(ShadowValue* shadowVal, double actualVal){
  // We're going to do the log in mpfr since that way we don't have to
  // worry about pulling in the normal math library, which is
  // non-trivial in a valgrind tool. But, we can't get ulps from an
  // mpfr value, so we have to first bring both values into doubles,
  // get their ulps, move the ulps into MPFR, get the +1log (which is
  // the number of bits), then finally convert that to double.
  long long ulpsError;
  double bitsError, shadowValD;
  mpfr_t ulpsErrorM, bitsErrorM;
  shadowValD = mpfr_get_d(shadowVal->value, MPFR_RNDN);

  VG_(printf)("The shadowed val is %f, and the actual val is %f.\n", shadowValD, actualVal);
  
  ulpsError = ulpd(shadowValD, actualVal);
  // To calculate bits error, we take the log2 of the ulps error +
  // 1. This means that 0 ulps (same value) has log2(1) = 0 bits of
  // error, 1 ulp (values that are as close as they can be but still
  // different) has log2(2) = 1 bit of error, and we scale
  // logarithmically from there.
  mpfr_init_set_si(ulpsErrorM, ulpsError + 1, MPFR_RNDN);
  mpfr_log2(bitsErrorM, ulpsErrorM, MPFR_RNDN);
  bitsError = mpfr_get_d(bitsErrorM, MPFR_RNDN);

  VG_(printf)("The bits error of that operation was: %f.\n", bitsError);
}

unsigned long long ulpd(double x, double y) {
  if (x == 0) x = 0; // -0 == 0
  if (y == 0) y = 0; // -0 == 0

  if (x != x && y != y) return 0;
  if (x != x) return LLONG_MIN; // Maximum error
  if (y != y) return LLONG_MIN; // Maximum error

  long long xx = *((long long*) &x);
  xx = xx < 0 ? LLONG_MIN - xx : xx;

  long long yy = *((long long*) &y);
  yy = yy < 0 ? LLONG_MIN - yy : yy;

  return xx >= yy ? xx - yy : yy - xx;
}

