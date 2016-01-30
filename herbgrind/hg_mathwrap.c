#include "pub_tool_clreq.h"
#include "herbgrind.h"
#include "hg_mathreplace_funcs.h"
#include "pub_tool_redir.h"

double VG_WRAP_FUNCTION_ZU(libmZdsoZd6, sqrt)(double x);
double VG_WRAP_FUNCTION_ZU(libmZdsoZd6, sqrt)(double x){
  double result;
  double args[1];
  args[0] = x;
  HERBGRIND_PERFORM_OP(OP_SQRT, &result, args);
  return result;
}
