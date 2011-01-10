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
 * libapreq headers
 */
#include <apreq.h>
#include <apreq_cookie.h>
#include <apreq_parser.h>
#include <apreq_param.h>
#include <apreq_util.h>

/*
 * R headers
 */
#define R_INTERFACE_PTRS
#include <R.h>
#include <Rinterface.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Error.h>


#define MAX_SERVER_LIMIT 200000


typedef struct ra_module_rec ra_module_rec_t;
typedef struct ra_server_rec ra_server_rec_t;

struct ra_module_rec {
	apr_pool_t *pool;
	ra_server_rec_t *server;

	/* R Interface variables and function that are saved while executing ralite */
	Rboolean R_Interactive;
	FILE *R_Consolefile;
	FILE *R_Outputfile;
	void (*ptr_R_Suicide)(const char *);
	void (*ptr_R_ShowMessage)(const char *);
	int  (*ptr_R_ReadConsole)(const char *, unsigned char *, int, int);
	void (*ptr_R_WriteConsole)(const char *, int);
	void (*ptr_R_WriteConsoleEx)(const char *, int, int);
	void (*ptr_R_ResetConsole)(void);
	void (*ptr_R_FlushConsole)(void);
	void (*ptr_R_ClearerrConsole)(void);
	void (*ptr_R_Busy)(int);
	void (*ptr_R_CleanUp)(SA_TYPE, int, int);
	int  (*ptr_R_ShowFiles)(int, const char **, const char **,
					   const char *, Rboolean, const char *);
	int  (*ptr_R_ChooseFile)(int, char *, int);
	int  (*ptr_R_EditFile)(const char *);
	void (*ptr_R_loadhistory)(SEXP, SEXP, SEXP, SEXP);
	void (*ptr_R_savehistory)(SEXP, SEXP, SEXP, SEXP);
	void (*ptr_R_addhistory)(SEXP, SEXP, SEXP, SEXP);

	/* R's signal handlers */
	void (*sig_int)(int);
	void (*sig_pipe)(int);
	void (*sig_usr1)(int);
	void (*sig_usr2)(int);

	/* ralite's interrupt flag */
	int pending_interrupt;

	/* Servers spawned */
	int server_pids_top;
	pid_t server_pids[MAX_SERVER_LIMIT];
};

struct ra_server_rec {
	apr_pool_t *pool;
	char *host;
	int port;
	SEXP handler;
	apr_socket_t *socket;
};

ra_module_rec_t *ralite;

static void become_ralite(ra_module_rec_t *ralite);
static void become_R(ra_module_rec_t *ralite);
static void spawn_servers(ra_module_rec_t *ralite,const char *host, 
	int port, SEXP handler, int numservers);
static void reap_servers(ra_module_rec_t *ralite);
static void dispatch_server(ra_module_rec_t *ralite, const char *host,int port,SEXP handler);

static void ra_handle_usr(int dummy){ signal(dummy,ra_handle_usr); }
static void ra_handle_pipe(int dummy){ signal(dummy,ra_handle_pipe); }
static void ra_handle_int(int dummy);

SEXP runRALITE(SEXP host, SEXP port, SEXP handler);

static void Suicide(const char *s){ };
static void ShowMessage(const char *s){ };
static int ReadConsole(const char *prompt, unsigned char *buf, int len, int hist){ };
static void WriteConsoleEx(const char *buf, int len, int otype){ };
static void WriteConsoleStderr(const char *, int, int);
static void WriteConsoleErrorOnly(const char *, int, int);
static void NoOpConsole(){ };
static void NoOpBusy(int i) { };
static void NoOpCleanUp(SA_TYPE s, int i, int j){ };
static int NoOpShowFiles(int i, const char **j, const char **k, const char *l, Rboolean b, const char *c){ return 1;};
static int NoOpChooseFile(int i, char *b,int s){ return 0;};
static int NoOpEditFile(const char *f){ return 0;};
static void NoOpHistoryFun(SEXP a, SEXP b, SEXP c, SEXP d){ };
