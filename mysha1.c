/* experimental SHA-1 hash */
/* standalone */
/* when compiling set either HAVE_ENDIAN (if endian.h is available)
 * or one of BIG_ENDIAN_MACHINE or LITTLE_ENDIAN_MACHINE */

#include <stdio.h>	/* fprintf, stderr */
#include <inttypes.h>
#include <string.h>	/* memcpy & friends */
#include "mysha1.h"

#ifdef HAVE_ENDIAN
# include <endian.h>	/* htobe32 & friends */
#else
# include "endian_lite.h"
#endif

/* simple rotation. n MUST be between 1 and 31 */
uint32_t rot32l(uint32_t v, uint32_t n)
{
	return (v<<n) | (v>>(32-n));
}

uint32_t rot32r(uint32_t v, uint32_t n)
{
	return (v>>n) | (v<<(32-n));
}

void sha1_proc_chunk(my_sha1_wd *wd)
{
	int i;
	uint32_t *w=(uint32_t *)wd->cc;
	uint32_t a=wd->h[0];
	uint32_t b=wd->h[1];
	uint32_t c=wd->h[2];
	uint32_t d=wd->h[3];
	uint32_t e=wd->h[4];
	uint32_t t,f,k;

#ifdef DEBUG
	printf("Got chunk:\n");
	for(i=0;i<64;i++)
		printf("%02x",wd->cc[i]);
	printf("\n");
#endif

	/* byte-swap */
	for(i=0;i<16;i++)
		w[i]=be32toh(w[i]);

	/* expand values in chunk buffer */
	for(i=16;i<80;i++)
		w[i] = rot32l((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]),1);

	/* calculate */
	for(i=0;i<80;i++)
	{
		if(i<20)
		{
			f = (b & c) | ((~b) & d);
			k = 0x5a827999;
		}
		else if(i<40)
		{
			f = b ^ c ^ d;
			k = 0x6ed9eba1;
		}
		else if(i<60)
		{
			f = (b & c) | (b & d) | (c & d);
			k = 0x8f1bbcdc;
		}
		else	/* to 80 */
		{
			f = b ^ c ^ d;
			k = 0xca62c1d6;
		}
		t = rot32l(a,5) + f + e + w[i] + k;
		e = d;
		d = c;
		c = rot32l(b,30);
		b = a;
		a = t;
	}
	wd->h[0]=wd->h[0]+a;
	wd->h[1]=wd->h[1]+b;
	wd->h[2]=wd->h[2]+c;
	wd->h[3]=wd->h[3]+d;
	wd->h[4]=wd->h[4]+e;
}

void my_sha1_init(my_sha1_wd *wd)
{
	wd->h[0]=0x67452301;
	wd->h[1]=0xefcdab89;
	wd->h[2]=0x98badcfe;
	wd->h[3]=0x10325476;
	wd->h[4]=0xc3d2e1f0;
	wd->cpos=0;
	wd->ml=0;
}

int my_sha1_addblock(my_sha1_wd *wd, uint8_t *d, uint32_t bl)
{
	uint32_t cp=wd->cpos;
	uint8_t *cc=wd->cc;
	uint32_t wl=bl+cp;

	while(wl>63)
	{
		memcpy(cc+cp,d,64-cp);
		cp=0;
		d+=64-cp;
		wl-=64;
		sha1_proc_chunk(wd);
	}

	if(wl)
	{
		memcpy(cc,d,wl);
		cp=wl;
	}
	wd->cpos=cp;
	if(wd->ml+(bl<<3) < wd->ml)
	{
		fprintf(stderr,"my_sha1_addblock: message length overflow. Can't compute SHA-1.\n");
		return 1;
	}
	wd->ml+=bl<<3;
	return 0;
}

int my_sha1_finalize(my_sha1_wd *wd, uint8_t *result)
{
	uint32_t *res=(uint32_t *)result;
	uint64_t bits=htobe64(wd->ml);
	uint8_t *cc=wd->cc;
	uint32_t cp=wd->cpos;

	/* final byte (in fact final bit, rounded up to byte size) */
	cc[cp++]=0x80;

	/* if we at most 56 bytes into cc, we have enough space to store ml
	 * (8 bytes) so this is the last chunk. if we're over 56 bytes, fill
	 * the rest with 0, process this and then another 0-filled one that
	 * only has ml at the end */
	if(cp<=56)	/* 56+8 = 64, meaning we have enough space to store ml in this chunk */
	{
		/* clear remaining buffer */
		if(cp!=56)
			memset(cc+cp,0,56-cp);
	}
	else
	{
		/* pad with 0s and process */
		memset(cc+cp,0,64-cp);
		sha1_proc_chunk(wd);

		/* empty chunk except for ml at the end */
		memset(cc,0,56);
	}
#ifdef DEBUG
	int i;
	printf("Final cp = %u, cc is:\n",cp);
	for(i=0;i<56;i++)
		printf("%02x",cc[i]);
	printf("\n");
#endif
	/* original ml in bits at the end */
	memcpy(cc+56,(uint8_t *)&bits,8);
	sha1_proc_chunk(wd);

	/* result */
	res[0]=htobe32(wd->h[0]);
	res[1]=htobe32(wd->h[1]);
	res[2]=htobe32(wd->h[2]);
	res[3]=htobe32(wd->h[3]);
	res[4]=htobe32(wd->h[4]);

	return 0;
}

/* calculate SHA1 digest of bl bytes from m and stores result in md */
/* now just a convenience wrapper */
void my_sha1(uint8_t *m, uint32_t bl, uint8_t *md)
{
	my_sha1_wd wd;

	my_sha1_init(&wd);
	my_sha1_addblock(&wd,m,bl);
	my_sha1_finalize(&wd,md);
}

/* compute hmac using sha1 as per RFC 2104 */
void my_hmac_sha1(uint8_t *k, uint32_t kl, uint8_t *m, uint32_t ml, uint8_t *h)
{
	unsigned int i;
	uint8_t ipad[64];	/* per RFC-2104 */
	uint8_t opad[64];	/* per RFC-2104 */
	uint8_t	kh[20];		/* key hash (used if key is longer than SHA1 block size, 64 bytes) */
	uint8_t th[20];		/* temporary hash */
	my_sha1_wd wd;

	/* initialize ipad, opad (could be initialized by hand, at declaration */
	memset(ipad,0,64);
	memset(opad,0,64);

	/* check kl: as per RFC-2104, if key is longer than hash block size,
	 * in this case 64 bytes, take the hash of the key and use that as key */
	if(kl>64)	
	{
		my_sha1(k,kl,kh);
		kl=20;
		k=kh;
	}
	/* copy key to both pads and XOR with initial values */
	memcpy(ipad,k,kl);
	memcpy(opad,k,kl);
	for(i=0;i<64;i++)
	{
		ipad[i]^=0x36;
		opad[i]^=0x5c;
	}

	/* save inner result to th */
	my_sha1_init(&wd);
	my_sha1_addblock(&wd,ipad,64);
	my_sha1_addblock(&wd,m,ml);
	my_sha1_finalize(&wd,th);

	/* outer */
	my_sha1_init(&wd);
	my_sha1_addblock(&wd,opad,64);
	my_sha1_addblock(&wd,th,20);
	my_sha1_finalize(&wd,h);
}
