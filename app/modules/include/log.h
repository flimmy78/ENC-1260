
#ifndef __LOG_H__
#define __LOG_H__

enum {
	DEBUG_ENABLE_NULL = 0,
	DEBUG_ENABLE_ERROR = 1,
	DEBUG_ENABLE_WARN = 2,
	DEBUG_ENABLE_PRINTF =3,
	DEBUG_ENABLE_DEBUG = 4
};

extern int TRACE_Print(const char* fmt, ...);
void trace_init(void);
int debug_shell_init(void);
int debug_level_set(int argc, char **argv);
int debug_level_get(int argc, char **argv);
int debug_output(int argc, char **argv);
int TRACE_Log(char *buf, int len);
void debug_set_all_level(int level);
void debug_set_syslog_level(int level);
int trace_get_debug_level(void);
int trace_set_debug_level( int flag);

#define PRINTF(X...)\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_PRINTF) {		\
		TRACE_Print("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		TRACE_Print(X);											\
	}														\
} while(0)

#define PRINTF_DELAY(X, Y...)								\
do {														\
	if(MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_PRINTF) {		\
		TRACE_Print("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		TRACE_Print(Y);											\
		mid_task_delay(X);									\
	}														\
}while( 0 )

#define PRINTF_ONLY(X...)									\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_PRINTF) {		\
		TRACE_Print(X);											\
	}														\
} while(0)

#define DBG_PRN(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_DEBUG) {		\
		TRACE_Print("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		TRACE_Print(X);											\
	}														\
} while(0)

#define WARN_PRN(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_WARN) {			\
		TRACE_Print("WARN! [%s:%s %d] ", __FILE__, __FUNCTION__, __LINE__);	\
		TRACE_Print(X);											\
	}														\
} while(0)
#define WARNNING WARN_PRN

#define WARN_OUT(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_WARN) {			\
		TRACE_Print("WARN! [%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
		TRACE_Print(X);											\
	}														\
	goto Warn;												\
} while(0)

#define ERR_PRN(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_ERROR) {		\
		TRACE_Print("ERROR! [%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
		TRACE_Print(X);											\
	}														\
} while(0)

#define ERR_OUT(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_ERROR) {		\
		TRACE_Print("ERROR! [%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
		TRACE_Print(X);											\
	}														\
	goto Err;												\
} while(0)

#define END_OUT(X...)										\
do {														\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_PRINTF) {		\
		TRACE_Print("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		TRACE_Print(X);											\
	}														\
	goto End;												\
} while(0)

#define RETURN(X)											\
do{															\
	if (MODULE_DEBUG_ENABLE >= DEBUG_ENABLE_ERROR) {		\
		TRACE_Print("ERROR! [%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
		return X;												\
	}														\
}while( 0 )

#endif //__STRM_DBG_H_2007_5_10__
