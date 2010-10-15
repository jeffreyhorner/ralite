#include "ralite.h"

#define RA_SAVE(x) ralite->x = x
#define RA_RESTORE(x) x = ralite->x
static void become_ralite(ra_module_rec *ralite){

	RA_SAVE(R_Interactive);
	RA_SAVE(R_Consolefile);
	RA_SAVE(R_Outputfile);
	RA_SAVE(ptr_R_Suicide);
	RA_SAVE(ptr_R_ShowMessage);
	RA_SAVE(ptr_R_WriteConsole);
	RA_SAVE(ptr_R_WriteConsoleEx);
	RA_SAVE(ptr_R_ReadConsole);
	RA_SAVE(ptr_R_ResetConsole);
	RA_SAVE(ptr_R_FlushConsole);
	RA_SAVE(ptr_R_ClearerrConsole);
	RA_SAVE(ptr_R_Busy);
	RA_SAVE(ptr_R_CleanUp);
	RA_SAVE(ptr_R_ShowFiles);
	RA_SAVE(ptr_R_ChooseFile);
	RA_SAVE(ptr_R_EditFile);
	RA_SAVE(ptr_R_loadhistory);
	RA_SAVE(ptr_R_savehistory);
	RA_SAVE(ptr_R_addhistory);

	R_Interactive = FALSE;
	R_Consolefile = NULL;
	R_Outputfile = NULL;
	ptr_R_Suicide = Suicide;
	ptr_R_ShowMessage = ShowMessage;
	ptr_R_WriteConsole = NULL;
	ptr_R_WriteConsoleEx = WriteConsoleEx;
	ptr_R_ReadConsole = ReadConsole;
	ptr_R_ResetConsole = ptr_R_FlushConsole = ptr_R_ClearerrConsole = NoOpConsole;
	ptr_R_Busy = NoOpBusy;
	ptr_R_CleanUp = NoOpCleanUp;
	ptr_R_ShowFiles = NoOpShowFiles;
	ptr_R_ChooseFile = NoOpChooseFile;
	ptr_R_EditFile = NoOpEditFile;
	ptr_R_loadhistory = ptr_R_savehistory = ptr_R_addhistory = NoOpHistoryFun;

	ralite->sig_int = signal(SIGINT,ra_handle_int);
	ralite->sig_usr1 = signal(SIGUSR1,ra_handle_usr);
	ralite->sig_usr2 = signal(SIGUSR2,ra_handle_usr);
#ifndef WIN32
	ralite->sig_pipe = signal(SIGPIPE,ra_handle_pipe);
#endif
}

static void become_R(ra_module_rec *ralite){

	RA_RESTORE(R_Interactive);
	RA_RESTORE(R_Consolefile);
	RA_RESTORE(R_Outputfile);
	RA_RESTORE(ptr_R_Suicide);
	RA_RESTORE(ptr_R_ShowMessage);
	RA_RESTORE(ptr_R_WriteConsole);
	RA_RESTORE(ptr_R_WriteConsoleEx);
	RA_RESTORE(ptr_R_ReadConsole);
	RA_RESTORE(ptr_R_ResetConsole);
	RA_RESTORE(ptr_R_FlushConsole);
	RA_RESTORE(ptr_R_ClearerrConsole);
	RA_RESTORE(ptr_R_Busy);
	RA_RESTORE(ptr_R_CleanUp);
	RA_RESTORE(ptr_R_ShowFiles);
	RA_RESTORE(ptr_R_ChooseFile);
	RA_RESTORE(ptr_R_EditFile);
	RA_RESTORE(ptr_R_loadhistory);
	RA_RESTORE(ptr_R_savehistory);
	RA_RESTORE(ptr_R_addhistory);

	signal(SIGINT,ralite->sig_int);
	signal(SIGUSR1,ralite->sig_usr1);
	signal(SIGUSR2,ralite->sig_usr2);
#ifndef WIN32
	signal(SIGPIPE,ralite->sig_pipe);
#endif
}

static void ra_handle_int(int dummy){ 
	ralite->pending_interrupt=1; 
	signal(SIGINT,ra_handle_int);
}

#ifndef WIN32
void spawn_servers(ra_module_rec *ralite,char *host, 
		int port, SEXP handler, int numservers ){
	int i, pid;

	if (numservers > MAX_SERVER_LIMIT){
		stop("Too many servers! Try a bit less.");
	}

	for (i=0; i<numservers;i++){
		if ((pid = fork()) < 0){
			/* FATAL ERROR */
		}
		if (pid == 0){
			become_ralite(ralite);
			dispatch_server(ralite,host,port,handler);
			exit(0);
		} else {
			ralite->server_pids[ralite->server_pids_top] = pid;
			ralite->server_pids_top++;
		}
	}
}
#endif

void dispatch_server(ra_module_rec *ralite, char *host, int port, SEXP handler){
	apr_pool_t *newpool;
	apr_status_t rv;
	ra_server_rec *server;
	apr_socket_t *socket;
	apr_sockaddr_t *listenaddr;

	if (apr_pool_create(&newpool,ralite->pool) != APR_SUCCESS){
		Free(ralite);
		fprintf(stderr,"FATAL ERROR! apr_pool_create failed!");
		return();
	}

	server = apr_palloc(newpool,sizeof(ra_server_rec));
	server->host = apr_pstrdup(newpool,host);
	server->port = port;
	server->handler = R_PreserveObject(handler);
	server->pool = newpool;
	ralite->server = server;

	rv = apr_socket_create(&server->socket, APR_INET, SOCK_STREAM,
			APR_PROTO_TCP, server->pool);
	if (rv != APR_SUCCESS){
		fprintf(stderr,"Socket allocation failed!!!\n")
		apr_pool_destry(server->pool);
		return();
	}

	rv = apr_sockaddr_info_get(&listenaddr, server->host, APR_UNSPEC, server->port,
			APR_IPV4_ADDR_OK, server->pool);
	if (rv != APR_SUCCESS){
		fprintf(stderr,"apr_sockaddr_info_get failed for %s!!!\n",server->host);
		apr_pool_destry(server->pool);
		return();
	}

	rv = apr_socket_bind(server->socket,listenaddr);
	if (rv != APR_SUCCESS){
		fprintf(stderr,"Bind failed!!!\n");
		apr_pool_destry(server->pool);
		return();
	}

	rv = apr_socket_listen(server->socket,0);
	if (rv != APR_SUCCESS){
		fprintf(stderr,"Listen failed!!!\n");
		apr_pool_destry(server->pool);
		return();
	}

	for (;;){
		rv = apr_socket_accept(&socket,server->socket,server->pool);
		if (ret != APR_SUCCESS){
			break;
		}
	}

	apr_pool_destroy(server->pool);
}

void reap_servers(ra_module_rec *ralite){
}
     
SEXP runRALITE(SEXP host, SEXP port, SEXP handler){
	int bg=0;
	int numservers=1;


	/* We're just going to fork a bunch of servers so no reason to
	 * redirect R's internals
	 */
	if (bg){
#ifndef WIN32
		spawn_servers(ralite,host,port,handler,numservers);
#else
		stop("Cannot fork on Windows!");
#endif
		return R_NilValue;
	} 
	
	/* Setup signal handlers*/
	/* Redirect R callbacks */
	become_ralite(ralite);

	dispatch_server(ralite,host,port,handler);

	/* Remove signal handlers*/
	/* Replace R callbacks */
	become_R(ralite);

	return R_NilValue;
}

R_CallMethodDef callMethods[]  = {
	{"runRALITE", (DL_FUNC) &runRALITE, 3},
	{NULL, NULL, 0}
};

void R_init_ralite(DllInfo *info) {
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_useDynamicSymbols(info, FALSE);

	apr_initialize();

	ralite = Calloc(1,ra_module_rec);

	if (apr_pool_create(&ralite->pool,NULL) != APR_SUCCESS){
		Free(ralite);
		stop("FATAL ERROR! apr_pool_create failed!");
	};

	apreq_initialize(ralite->pool);
}
     
void R_unload_ralite(DllInfo *info) {
	apr_pool_destroy(ralite->pool);
	Free(ralite);
}
