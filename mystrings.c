#ifndef __MYSTRINGS_C__
#define __MYSTRINGS_C__
/* this needs the following: */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mystrings.h"


#define MYS_EOF 2
#define MYS_EOF_NODATA 4
#define MYS_LINETOOLONG 3

/* str_repl helpers */

/* free occurences list (see str_repl) */
void freeocc(strocc *f)
{
	if(f==NULL) return;
	strocc *c,*o=f;
	while((c=o->n)!=NULL)
	{
		free(o);
		o=c;
	}
	free(o);
}

/* store occurence */
strocc *storeocc(strocc *c, char *p)
{
	strocc *nocc=calloc(1,sizeof(strocc));
	if(nocc==NULL) return NULL;
	nocc->sp=p;
	if(c!=NULL) c->n=nocc;
	return nocc;
}

/* returns new string copied from src with every occurence of s replaced by r
 * returns NULL if src, s or r are NULL, or if any malloc fails
 * caller must free the result */
char *str_repl(char *src, char *s, char *r)
{
	if(src==NULL || s==NULL || s[0]==0 || r==NULL) return NULL;
	strocc *focc=NULL,*cocc=NULL;
	char *d,*dst;
	char *t;
	char *csrc=src;
	int srcl=strlen(src);
	int sl=strlen(s);
	int rl=strlen(r);
	int dl=rl-sl; /* d for delta, not dest */
	int tdl=0; /* total delta */
	while((t=strstr(csrc,s))!=NULL)
	{
		if((cocc=storeocc(cocc,t))==NULL)
		{
			freeocc(focc);
			return NULL;
		}
		if(focc==NULL) focc=cocc;
		csrc=t+sl;
		tdl+=dl;
	}

	if(focc==NULL)
	{
		d=malloc(srcl+1);
		if(d==NULL) return NULL;
		memcpy(d,src,srcl+1);
		return d;
	}

	cocc=focc;
	d=malloc(srcl+tdl+1);
	if(d==NULL) { freeocc(focc); return NULL; }
	dst=d;
	t=src;
	int l;
	while(cocc!=NULL)
	{
		l=cocc->sp-t;
		memcpy(d,t,l);
		memcpy(d+l,r,rl);
		d+=l+rl;
		t=cocc->sp+sl;
		cocc=cocc->n;
	}
	strcpy(d,t);
	freeocc(focc);
	return dst;
}


/* mys_readline (from file) related functions */
/* close file and free buffers */
void mys_close(mys_finfo *fi)
{
	close(fi->fd);
	free(fi->buf);
	free(fi);
}

/* opens file and initializes mys_finfo structure for mys_readline */
mys_finfo *mys_open(char *fn, size_t bsize)
{
	if(fn==NULL)
	{
		fprintf(stderr,"mys_open: File name is NULL\n");
		return NULL;
	}
	if(!bsize)
	{
		fprintf(stderr,"mys_open: bsize is 0\n");
		return NULL;
	}
	mys_finfo *fi=calloc(1,sizeof(struct mys_finfo));
	if(fi==NULL)
	{
		fprintf(stderr,"mys_open: alloc error\n");
		return NULL;
	}
	if((fi->buf=malloc(bsize))==NULL)
	{
		fprintf(stderr,"mys_open: alloc error\n");
		free(fi);
		return NULL;
	}
	fi->bsize=bsize;
	if((fi->fd=open(fn,O_RDONLY))<0)
	{
		fprintf(stderr,"mys_open: unable to open %s\n",fn);
		free(fi->buf);
		free(fi);
		return NULL;
	}
	fi->bvalid=0;
	fi->ateof=0;
	return fi;
}

/* positions file pointer at pos, invalidates buffer.
 * Returns 0 on success or -1 on error. */
int mys_seek(mys_finfo *fi, off_t pos)
{
	fi->bpos=fi->buf;
	fi->bvalid=0;
	fi->ateof=0;
	if(lseek(fi->fd,pos,SEEK_SET)<0)
		return -1;
	return 0;
}

/* moves valid bytes from bpos to buf, fills up buf, resets bpos to buf.
 * Returns no. of bytes read or -1 on read error. */
off_t mys_reset_fi(mys_finfo *fi)
{
	off_t nr;
	if(fi->bvalid && fi->buf!=fi->bpos)
		memmove(fi->buf,fi->bpos,fi->bvalid);	/* move start of line at beginning of buf */

	fi->bpos=fi->buf;							/* reset position */
	if((nr=read(fi->fd,fi->buf+fi->bvalid,fi->bsize-fi->bvalid))<0) /* read remainder of buf */
	{
		fprintf(stderr,"mys_reset_fi: read error\n");
		return -1;
	}
	fi->bvalid+=nr;
	if(nr) fi->ateof=0; else fi->ateof=1;
	return nr;
}

ssize_t mys_search_nl(mys_finfo *fi)
{
	char *nlp;
	if(fi->bvalid==0)		/* fill some bytes */
		mys_reset_fi(fi);
	if((nlp=memchr(fi->bpos,'\n',fi->bvalid))==NULL)
	{ /* if we haven't found \n try to fill some more bytes and search again */
		mys_reset_fi(fi);
		nlp=memchr(fi->bpos,'\n',fi->bvalid);
	}
	if(nlp!=NULL) return nlp-fi->bpos;
	return -1;
}


/* return number of lines in buf; if ll!=NULL it will be loaded with longest line length
 * if buf is NULL, returns 0 and ll is untouched */
unsigned int countbuf(char *buf, unsigned int *ll)
{
	unsigned int n=0,l=0;
	char *x;
	if(buf==NULL) return 0;
	while((x=strchr(buf,'\n'))!=NULL)
	{
		n++;
		if((unsigned int)(x-buf)>l)	/* cast because x can't be smaller than buf at this point */
			l=x-buf;
		buf=x+1;
	}
	if(!n)
		l=strlen(buf);
	if(ll!=NULL) *ll=l;
	return n;
}

/* replaces \n with \0 in buf and stores pointers to the beginning of each line in lines[] array
 * lines[] is allocated by buf2lines, must be freed by the caller
 * returns number of lines on success or -1 on failure
 * it should work with LF, CR and CRLF line endings. */
int buf2lines(char *buf, char **lines[])
{
	int n=0,i=0; /* n = number of lines */
	strocc *focc=NULL,*cocc=NULL;
	char *prev=buf;
	char **tlines;
	if(buf==NULL) return -1;
	while(buf[i]!='\0')
	{
		if(buf[i]=='\r' && buf[i+1]=='\n')	/* get rid of lurking CR in CRLF */
		{
			buf[i++]=0;
			continue;
		}
		if(buf[i]=='\n' || buf[i]=='\r')
		{
			if((cocc=storeocc(cocc,prev))==NULL) goto allocfail;
			if(focc==NULL) focc=cocc;
			buf[i]=0;
			prev=buf+i+1;
			n++;
		}
		i++;
	}
	if(i==0)	/* buf was empty */
	{
		*lines=NULL;
		return 0;
	}
	else if(buf[i-1]!=0)	/* last line did not end with \n (replaced with \0) */
	{
		if((cocc=storeocc(cocc,prev))==NULL) goto allocfail;
		if(focc==NULL) focc=cocc;
		n++;
	}

	tlines=calloc(n,sizeof(char *));
	if(tlines==NULL)
	{
		freeocc(focc);
		return -1;
	}

	cocc=focc;
	for(i=0;i<n;i++)
	{
		tlines[i]=cocc->sp;
		cocc=cocc->n;
	}
	freeocc(focc);
	*lines=tlines;
	return n;
allocfail:
	fprintf(stderr,"buf2lines: malloc failed\n");
	freeocc(focc);
	return -1;
}


/* searches for \n forward from current position and copies at most n-1 bytes into d
 * adding a terminating null byte and updating relevant info in fi.
 * constraint: n<=bsize, enforced. */
int mys_readline(char *d, size_t n, mys_finfo *fi, int notrunc)
{
	/* checks (valid pointers, constrain n to bsize) */
	if(d==NULL || fi==NULL) { fprintf (stderr,"mys_readline: NULL pointer error!\n"); return -1; }
	if(n>fi->bsize) { fprintf (stderr,"requested line longer than buffer. constraining to %zu\n",fi->bsize); n=fi->bsize; }
	if(n==0) return 0; /* do nothing */

	ssize_t ll; /* line length */
	ll=mys_search_nl(fi);
	if(ll<0)
	{
		if(notrunc)
		{
			d[0]=0;
			if(fi->ateof) return MYS_EOF_NODATA;
			else return MYS_LINETOOLONG;
		}
		ll=fi->bvalid;				/* if no nl found, get all valid bytes */
	}
	if(ll>= (ssize_t)n) ll=n-1;				/* but not more than destination size */
	if(ll) memcpy(d,fi->bpos,ll);
	d[ll]=0;
	fi->bpos+=ll; fi->bvalid-=ll;
	if(fi->bvalid>0 && fi->bpos[0]=='\n')	/* would this work on windows? */
	{
		fi->bpos++; fi->bvalid--;
	}
	if(fi->ateof) return MYS_EOF;
	return 0;
}

/* used for http headers, but might be useful in other cases as well?
 * not the best name... */
char *mys_get_hvalue(char *lines[],int n,char *rh)
{
	int i;
	size_t sl;
	char *crt;
	if(lines==NULL || n==0 || rh==NULL) return NULL;
	sl=strlen(rh);
	for(i=0;i<n;i++)
	{
		if(memcmp(lines[i],rh,sl)==0)
		{
			/* check that header continues with ':' */
			crt=lines[i]+sl;
			if(*crt++!=':') continue;
			/* skip spaces after ':' and return result */
			while(*crt==' ') crt++;
			return crt;
		}
	}
	return NULL;
}



/* other file related functions */

/* read whole fn into a (NULL terminated) buffer
 * returns number of bytes read or -1 for all errors */
ssize_t file2lbuf(char *fn, char **dbuf)
{
	if(fn==NULL)
	{
		fprintf(stderr,"file2lbuf: File name is NULL!\n");
		return -1;
	}
	struct stat finfos;
	char *buf;
	int fd=open(fn,O_RDONLY);
	if(fd<1)
	{
		/* ifdef to avoid unneeded file not found messages for legitimate reasons */
#ifdef DEBUG1
		fprintf(stderr,"file2lbuf: Unable to open %s\n",fn);
#endif
		return -1;
	}
	if(fstat(fd,&finfos)!=0)
	{
		fprintf(stderr,"file2lbuf: Unable to stat %s\n",fn);
		return -1;
	}
	off_t isize=finfos.st_size;
	if(isize>MAXFILELIMIT)
	{
		fprintf(stderr,"file2lbuf: %s file size above limit.\n(MAXFILELIMIT=%d)",fn,MAXFILELIMIT);
		return -1;
	}
	if(isize==0)
	{
		fprintf(stderr,"file2lbuf: %s file size is 0\n",fn);
		close(fd);
		return 0;
	}
	buf=malloc(isize+1);
	off_t r=read(fd,buf,isize);
	close(fd);
	if(r!=isize)
	{
		fprintf(stderr,"file2lbuf: Unable to read (whole?) %s\n",fn);
		free(buf);
		return -1;
	}
	buf[isize]=0;
	*dbuf=buf;
	return isize;
}

/* read whole fn into a (NULL terminated) buffer
 * returns NULL for all errors
 * see file2lbuf for function returning length */
char *file2buf(char *fn)
{
	char *buf;
	if(file2lbuf(fn,&buf)>0) return buf;
	else return NULL;
}

/* write buf to file, passing of and cm to open(2) 
 * returns 0 on success */
int buf2file(char *buf, char* fn, int l, int of, int cm)
{
	int r;
	if(fn==NULL)
	{
		fprintf(stderr,"File name is NULL in buf2file!\n");
		return -1;
	}
	int fd=open(fn,of,cm);
	if(fd<1)
	{
		fprintf(stderr,"Unable to open or create %s\n",fn);
		return -1;
	}
	r=write(fd,buf,l);
	close(fd);
	if(r<0)
	{
		fprintf(stderr,"Write error for %s\n",fn);
		return -1;
	}
	return 0;
}


/* hex2bin and friends */
/* return value of single hex digit or -1 if invalid digit */
int h2b(int c)
{
	if(c>0x60) c-=0x20;
	c-=0x30;
	if(c<0||(c>0x09 && c<0x11)||c>0x16) return -1; /* invalid digit */
	if(c>0x09) c-=7;
	return c;
}

/* reads up to 8 hexadecimal digits and returns their binary (dword/uin32_t) representation
 * reading stops on first non-digit character or after 8 digits
 * if l is not NULL, it will receive number of digits read */
uint32_t hexn2dw(char *src, int *l)
{
	uint32_t r=0;
	int t;
	int dr=0;
	
	while(((t=h2b(src++[0]))>=0) && dr<8)
	{
		r=(r<<4)+t;
		dr++;
	}
	if(l!=NULL) *l=dr;
	return r;
}

/* converts a null terminated string of hex digits to byte string.
 * result is malloc-ed and null terminated; allocated size can be 
 * larger. (it assumes only hex digits in src)
 * non-hex digits (0-9,A-F,a-f) in src are ignored
 * returns number of bytes written to dst (not including terminator)
 * or -1 on error (malloc fail)
 * if ld is not NULL it will contain 0 if an even number of digits was found or
 * the ASCII code of the last digit if an odd number of digits was found in src
 */
int hex2bin(char **lpdst, char *src, char *ld)
{
	int l=strlen(src);
	int i=0,di=0,cp=1;
	char *dst=calloc(1+l/2,sizeof(char)); /* dst *must* be zeroed */
	int c;
	char lgc=0;

	if(dst==NULL) return -1;
	*lpdst=dst;
	while(i<l)
	{
		if((c=h2b(src[i]))>=0)
		{
			lgc=src[i];
			dst[di]+=(c<<(cp*4));
			cp=cp^1;
			di+=cp;
		}
		i++;
	}
	if(cp==1) lgc=0;
	if(ld!=NULL) ld[0]=lgc;
	return di;
}

/* converts len bytes from src to hex digits in dst.
 * Does not add terminating NULL */
void b2h(char *dst, char *src, int len)
{
	int i=0,j=0;
	unsigned char c;
	while(i<len)
	{
		c=(src[i]&0xf0)>>4;
		if(c>9) c+=0x41-10; else c+=0x30;
		dst[j]=c;
		j++;
		c=src[i]&0x0f;
		if(c>9) c+=0x41-10; else c+=0x30;
		dst[j]=c;
		j++;
		i++;
	}
}

/* converts len bytes from src to hex digits in dst. mallocs dst including null terminator
 * returns number of chars written (not including terminating NULL) */
int bin2hex(char **lpdst, char *src, int len)
{
	int l=1+len*2;/* 2 digits per byte plus terminator */
	char *dst=malloc(l); 
	if(dst==NULL) return -1;
	b2h(dst,src,len);
	dst[2*len]=0;
	*lpdst=dst;
	return 2*len;
}

/* dump visible ASCII chars, similar to right-hand side of hexdump -C
 * used by dump_hex, below */
void dump_chars(unsigned char *buf, unsigned int pos, unsigned int size)
{
	int i;
	unsigned char c;
	putchar('|');
	for(i=0;i<16;i++)
	{
		if(pos<size)
			c=buf[pos];
		if(pos<size && c>0x1f && c<0x7f)
			putchar(c);
		else
			putchar('.');
		pos++;
	}
	putchar('|');
}

/* hex dump of buf to STDOUT, a la hexdump -C */
void dump_hex(void *xbuf, unsigned int size)
{
	unsigned char *buf=xbuf;
	unsigned int cp=0,lp=0;
	int i;
	if(buf==NULL || size<=0) return;
	while(cp<size)
	{
		if(lp==0)
			printf("%08x ",cp);
		else if(lp==8)
			printf(" ");
		printf(" %02x",buf[cp]);
		cp++;
		lp++;
		if(lp==16)
		{
			lp=0;
			printf("  ");
			dump_chars(buf,cp-16,size);
			printf("\n");
		}
	}
	if(lp!=0)	/* complete line */
	{
		for(i=lp;i<16;i++)
			printf("   ");
		if(lp<8) printf(" ");
		printf("  ");
		dump_chars(buf,cp-lp,size);
		printf("\n");
	}
}


/* slices lines longer than ll in more lines (assuming sep contains \n)
 * of at most ll (not including sep), using sep.
 * both CR and LF are considered line terminators
 * returns number of bytes written to destination not including NULL terminator
 * or a negative value in case of error */
int slice_lines(char **lpdst, char *src, int ll, char *sep)
{
	int i;
	int l=strlen(src);
	int spl=strlen(sep);
	int dl=l+spl*l/ll; /* assume maximum no. of slices (no linebreaks in file) */
	int clp=0,cdp=0; /* current line position, current dest position */
	char *dst=malloc(1+dl);
	if(dst==NULL) return -1;
	for(i=0;i<l;i++)
	{
		if(clp>=ll)
		{
			memcpy(dst+cdp,sep,spl);
			clp=0;
			cdp+=spl;
		}
		dst[cdp]=src[i];
		cdp++;
		if(src[i]==10 || src[i]==13)
			clp=0;
		else
			clp++;
	}
	dst[cdp]=0;
	*lpdst=dst;
	return cdp;
}

/* trims spaces from end of s (modifies s!) */
void rtrim(char *s)
{
	int i=strlen(s);
	while(i>0 && s[--i]==0x20);
	if(s[i+1]==0x20) s[i+1]=0;
	if(i==0 && s[0]==0x20) s[0]=0;
}

/* ASCII convert to UPPER CASE */
void upper(char *s)
{
	while(*s!=0)
	{
		if(*s >= 'a' && *s <= 'z')
			*s&=0xdf;
		s++;
	}
}

/* ASCII convert to lower case */
void lower(char *s)
{
	while(*s!=0)
	{
		if(*s >= 'A' && *s <= 'Z')
			*s|=0x20;
		s++;
	}
}


void nupper(char *s, unsigned int l)
{
	while(l--)
	{
		if(*s >= 'a' && *s <= 'z')
			*s&=0xdf;
		s++;
	}
}

void nlower(char *s, unsigned int l)
{
	while(l--)
	{
		if(*s >= 'A' && *s <= 'Z')
			*s|=0x20;
		s++;
	}
}


size_t memcpyunesc(char *dst, char *src, size_t l, int *isNULL)
/* copies l bytes from src to dest "unescaping" \prefixed characters.
 * does not check for, or add NULL terminators
 * if l==0 nothing is written to dst and returns 0
 * if src starts with \Z it is interpreted as NULL and 0 is returned
 * if isNULL is a valid pointer is set to 1 if src starts with \Z, 0 otherwise
 * returns number of bytes written to dst */
{
	if(src==NULL || dst==NULL) return 0;
	if(l==0)
	{
		if(isNULL!=NULL)
			*isNULL=0;
		return 0;
	}
	size_t i,j=0;
	char c;
	int b=0;
	for(i=0;i<l;i++)
	{
		c=src[i];
		if(b)
		{
			if(i==1 && src[i]=='Z') /* special NULL marker \Z only at the beginning */
			{
				if(isNULL!=NULL)
					*isNULL=1;
				return 0;
			}
			dst[j++]=c;
			b=0;
			continue;
		}
		if(c=='\\')
		{
			b=1;
			continue;
		}
		dst[j++]=c;
	}
	if(isNULL!=NULL)
		*isNULL=0;
	return j;
}

/* extend dst (using realloc) with strlen(src), 
 * performs strcat, returns dst (and updates its pointer) */
char *astrcat(char **dst, char *src)
{
	if(dst==NULL || src==NULL) return NULL;
	char *nd;
	size_t ld=0;
	size_t ls=strlen(src);
	if(*dst!=NULL) ld+=strlen(*dst);	/* TODO: = instead of += ? */
	nd=realloc(*dst,ls+ld+1);
	if(nd==NULL) return NULL;
	memcpy(nd+ld,src,ls);
	nd[ld+ls]=0;
	*dst=nd;
	return nd;
}

/* determine if ch is in printable ASCII set */
int ch_printable(int ch)
{
	if(ch>=32 && ch<=126)
		return 1;
	else
		return 0;
}
#endif
