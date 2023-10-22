/* helper macros */
/* FUNC_ABORT_NA (no arguments) has to be used because with Pelles VA_ARGS (...) require at least one argument */
#ifdef WIN32_UTF16	/* in fact this is for all win32 */
# define FUNC_ABORT_NA(msg) \
	do { fprintf(stderr,"%s: " msg "\n",__func__); return -1; } while (0)
# define FUNC_ABORT(msg, ...) \
	do { fprintf(stderr,"%s: " msg "\n",__func__, __VA_ARGS__); return -1; } while (0)
# define FUNC_MSG(msg, ...) \
	do { fprintf(stderr,"%s: " msg "\n",__func__, __VA_ARGS__); } while (0)
#else
# define FUNC_ABORT_NA(msg) \
	do { fprintf(stderr,"%s: " msg "\n",__func__); return -1; } while (0)
# define FUNC_ABORT(msg, ...) \
	do { fprintf(stderr,"%s: " msg "\n",__func__, ##__VA_ARGS__); return -1; } while (0)
# define FUNC_MSG(msg, ...) \
	do { fprintf(stderr,"%s: " msg "\n",__func__, ##__VA_ARGS__); } while (0)
/* very specific to my sources. function has a label fail: that frees
 * allocated resources before returning -1 */
# define FUNC_FAIL(msg, ...) \
	do { fprintf(stderr,"%s: " msg "\n",__func__, ##__VA_ARGS__); goto fail; } while (0)
#endif

#define CLR_STRUCT(s) memset(&s,0,sizeof(s))

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define Str(x) #x
#define VStr(x) Str(x)
