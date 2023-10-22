#ifndef __MYSTRINGS_H__
#define __MYSTRINGS_H__

#include <inttypes.h>
#include <unistd.h>


/* max file size for file2buf */
#define MAXFILELIMIT 1024*1024*2


typedef struct strocc strocc;
struct strocc
{
	char *sp;
	strocc *n;
};

typedef struct mys_finfo /* used by mys_getline */
{
	int fd;
	char *buf;
	char *bpos;
	size_t bsize;
	size_t bvalid; /* this many bytes are "valid" relative to bpos - useful at EOF */
	int ateof;
} mys_finfo;

/* mys_readline from file; other line-related */
mys_finfo *mys_open(char *fn,size_t bsize);
void mys_close(mys_finfo *fi);
int mys_seek(mys_finfo *fi, off_t pos);
off_t mys_reset_fi(mys_finfo *fi);
ssize_t mys_search_nl(mys_finfo *fi);

int buf2lines(char *buf, char **lines[]);
int mys_readline(char *d,size_t n,mys_finfo *fi, int notrunc);
/* search lines array for line starting with rh followed by ':' and returns a pointer to
 * the string after ':' (skipping over spaces after ':') */
char *mys_get_hvalue(char *lines[],int n,char *rh);
int slice_lines(char **lpdst, char *src, int ll, char *sep);

/* file2buffer (with/without length) and reverse */
ssize_t file2lbuf(char *fn, char **dbuf);
char *file2buf(char *fn);
int buf2file(char *buf, char* fn, int l, int of, int cm);

/* hex2bin, bin2hex, dumphex */
int h2b(int c);
uint32_t hexn2dw(char *src,int *l);
int hex2bin(char **lpdst, char *src, char *ld);
void b2h(char *dst, char *src, int len);
int bin2hex(char **lpdst, char *src, int len);
void dump_chars(unsigned char *buf, unsigned int pos, unsigned int size);
void dump_hex(void *buf, unsigned int size);

/* other string ops */
void rtrim(char *s);
void upper(char *s);
void lower(char *s);
void nupper(char *s, unsigned int l);
void nlower(char *s,unsigned int l);
int ch_printable(int ch);
unsigned int countbuf(char *buf, unsigned int *ll);

/* unescape */
size_t memcpyunesc(char *dst, char *src, size_t l, int *isNULL);

/* concatenation */
char *astrcat(char **dst, char *src);

/* replace
 * returns new string copied from src with every occurence of s replaced by r
 * returns NULL if src, s or r are NULL, or if any malloc fails
 * caller must free the result */
char *str_repl(char *src, char *s, char *r);

#endif
