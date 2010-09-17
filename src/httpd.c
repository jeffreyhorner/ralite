/*
 * Apache Portable Runtime
 */
#include <apr.h>
#include <apr_network_io.h>
#include <apr_errno.h>
#include <apr_strings.h>
#include <apr_env.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_thread_mutex.h>

/*
 * R headers
 */
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
     
SEXP runHTTPD(SEXP host, SEXP port, SEXP handler){
	return NULL;
}

R_CallMethodDef callMethods[]  = {
	{"runHTTPD", (DL_FUNC) &runHTTPD, 3},
	{NULL, NULL, 0}
};

void R_init_httpd(DllInfo *info) {
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_useDynamicSymbols(info, FALSE);

	apr_initialize();
}
     
void R_unload_httpd(DllInfo *info) {
}
