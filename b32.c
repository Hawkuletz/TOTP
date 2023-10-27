#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "genmacros.h"

static char b32enctable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";

static unsigned char b32dectable[]=
{
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 00 */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 08 */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 10 */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 18 */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 20 */
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,/* 28 */
	0xff,0xff,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,/* 30 */
	0xff,0xff,0xff,0xff,0xff,0x00,0xff,0xff,/* 38 */
	0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,/* 40 */
	0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,/* 48 */
	0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,/* 50 */
	0x17,0x18,0x19,0xff,0xff,0xff,0xff,0xff,/* 58 */
	0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,/* 60 */
	0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,/* 68 */
	0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,/* 70 */
	0x17,0x18,0x19,0xff,0xff,0xff,0xff,0xff /* 78 */
};

/* bpos options:
 * 0 xxxxx000
 * 1 0xxxxx00
 * 2 00xxxxx0
 * 3 000xxxxx
 * 4 0000xxxx-x0000000
 * 5 00000xxx-xx000000
 * 6 000000xx-xxx00000
 * 7 0000000x-xxxx0000
 *
 * 0 		  xxxxx000	   f8
 * 5 00000xxx-xx000000	07-c0
 * 2		  00xxxxx0	   3e
 * 7 0000000x-xxxx0000	01-f0
 * 4 0000xxxx-x0000000	0f-80
 * 1 		  0xxxxx00	   7c
 * 6 000000xx-xxx00000	03-e0
 * 3 		  000xxxxx	   1f
 */

void b32_store(char *d,uint8_t s)
{
	*d=b32enctable[s];
}

int b32_dec(uint8_t *dst, char *s, size_t l, size_t *dl)
{
	uint8_t *d=dst;
	unsigned int bpos=0;
	uint8_t c,v;
	uint8_t db=0;	/* holder for partially decoded byte */
	if(d==NULL || s==NULL)
		FUNC_ABORT_NA("src or dst is NULL");

	while(l--)
	{
		c=(unsigned char)*s++;
		if(c&0x80) continue;	/* ignore chars with upper bit set (maybe raise an error?) */
		v=b32dectable[c];		/* decode value */
		if(v==0xff) continue;	/* ignore invalid chars (maybe only ignore separators?) */

		/* if an entire "quanta" has been decoded and we're on padding, stop */
		if(c=='=' && bpos==0)
			break;


		/* bits work (and storing) */
		switch(bpos)
		{
			case 0:	/* xxxxx000 */
				db=v<<3;
				bpos=5;
				break;
			case 1:	/* 0xxxxx00 */
				db|=v<<2;
				bpos=6;
				break;
			case 2:	/* 00xxxxx0 */
				db|=v<<1;
				bpos=7;
				break;
			case 3:	/* 000xxxxx */
				db|=v;
				*d++=db;
				bpos=0;
				break;
			case 4:	/* 0000xxxx x0000000 */
				db|=v>>1;
				*d++=db;
				db=v<<7;
				bpos=1;
				break;
			case 5:	/* 00000xxx xx000000 */
				db|=v>>2;
				*d++=db;
				db=v<<6;
				bpos=2;
				break;
			case 6:	/* 000000xx xxx00000 */
				db|=v>>3;
				*d++=db;
				db=v<<5;
				bpos=3;
				break;
			case 7:	/* 0000000x xxxx0000 */
				db|=v>>4;
				*d++=db;
				db=v<<4;
				bpos=4;
				break;
		}
	}
	if(dl)
		*dl=(d-dst);
	return 0;
}

int b32_enc(char *d, uint8_t *s ,size_t l)
{
	uint32_t bytepos=0;
	unsigned int bpos=0;
	uint8_t bl,br,pl;
	/*          0 1 2 3 4 5 6 7 */
	int pla[8]={0,3,6,1,4,7,2,5};
	if(d==NULL || s==NULL)
		FUNC_ABORT_NA("src or dst is NULL");

	while(1)
	{
		printf("bytepos=%d, bpos=%d\n",bytepos,bpos);
		/* exit conditions */
		if(bytepos>=l)
		{
			if(bpos==0)
				return 0;
			if(bpos>7) return -1; /* shouldn't happen */
			/* padding */
			pl=pla[bpos];
			while(pl--) b32_store(d++,32);
			return 0;
		}
		/* normal work */
		else
		{
			bl=s[bytepos];
			if(bytepos+1<l)
				br=s[bytepos+1];
			else
				br=0;
		}
		/* bits work (and storing) */
		switch(bpos)
		{
			case 0:
				b32_store(d++,(bl)>>3);
				bpos=5;
				break;
			case 1:
				b32_store(d++,((bl>>2)&0x1f));
				bpos=6;
				break;
			case 2:
				b32_store(d++,((bl>>1)&0x1f));
				bpos=7;
				break;
			case 3:
				b32_store(d++,bl&0x1f);
				bpos=0;
				bytepos++;
				break;
			case 4:
				b32_store(d++, (((bl<<1) | (br>>7)) & 0x1f ));
				bpos=1;
				bytepos++;
				break;
			case 5:
				b32_store(d++, (((bl<<2) | (br>>6)) & 0x1f ));
				bpos=2;
				bytepos++;
				break;
			case 6:
				b32_store(d++, (((bl<<3) | (br>>5)) & 0x1f ));
				bpos=3;
				bytepos++;
				break;
			case 7:
				b32_store(d++, (((bl<<4) | (br>>4)) & 0x1f ));
				bpos=4;
				bytepos++;
				break;
		}
	}
	return 0;
}

int b32_enca(char **dst,uint8_t *s,size_t l)
{
	size_t dlen;
	/* base32 is 8 symbols for each 5 bytes 
	 * ng=sl/5 ; if sl%5 then ng++
	 * dl=ng*8+1 */
	dlen=l/5;
	if(l%5)dlen++;
	/* sanity check; I know size_t is no longer 32bit but nevermind, don't want more than 4GB base32 */
	if(dlen>UINT_MAX/8) FUNC_ABORT_NA("Source buffer too large");
	dlen=dlen*8+1;	/* add room for null terminator */

	if(dst==NULL || s==NULL)	/* invalid arguments */
		FUNC_ABORT_NA("src or dst is NULL");

	char *buf=malloc(dlen);
	if(!buf)
		FUNC_ABORT_NA("alloc failed");

	b32_enc(buf,s,l);
	buf[dlen-1]=0;
	*dst=buf;
	return 0;
}

int b32_deca(uint8_t **dst,char *s,size_t l,size_t *dl)
{
	size_t al;
	size_t dlen=1+5*(l/8);		/* 1+ to add space for terminating NULL */
	if((l&0xf)!=0) dlen+=5;		/* in case some padding is missing, play it safe */
	if(dst==NULL || s==NULL)	/* invalid arguments */
		FUNC_ABORT_NA("src or dst is NULL");
	if(l==0)					/* nothing to do */
		return 0;
	uint8_t *buf=malloc(dlen);
	if(!buf)
		FUNC_ABORT_NA("alloc failed");

	b32_dec(buf,s,l,&al);
	buf[al]=0;
	*dst=buf;
	if(dl)
		*dl=al;
	return 0;
}
