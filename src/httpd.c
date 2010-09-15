#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
     
SEXP runHTTPD(SEXP host, SEXP port, SEXP handler){
}

R_CallMethodDef callMethods[]  = {
	{"runHTTPD", (DL_FUNC) &runHTTPD, 3},
	{NULL, NULL, 0}
};

void R_init_mylib(DllInfo *info) {
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
}
     
void R_unload_mylib(DllInfo *info) {
}
