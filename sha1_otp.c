/* experiment to check SHA1-OTP as per rfc4226 & rfc6238
 * helper functions
 * link with -lcrypto if using openssl */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_OPENSSL
# include <openssl/evp.h>
# include <openssl/hmac.h>
#elif defined(USE_OWN_SHA1)
# include "mysha1.h"
#else
# error "define either HAVE_OPENSSL or USE_OWN_SHA1"
#endif
#include "sha1_otp.h"

/* quad-word to 8byte string (big endian) */
void qw2s_be(unsigned char *dst,uint64_t e)
{
	int i;
	int sh=56;
	for(i=0;i<8;i++,sh-=8)
		dst[i]=(e>>sh) & 0xff;
}

/* quad-word to 8byte string (small endian) */
void qw2s_se(unsigned char *dst,uint64_t e)
{
	int i;
	int sh=0;
	for(i=0;i<8;i++,sh+=8)
		dst[i]=(e>>sh) & 0xff;
}

/* get dword from hs (truncate as per rfc4226)
 * hs is must be at least 20 bytes long */
uint32_t hstrunc(unsigned char *hs)
{
	uint32_t off,p;
	off=hs[19]&0x0f;
	p=(hs[off]<<24) | (hs[off+1]<<16) | (hs[off+2]<<8) | hs[off+3];
	return p&0x7fffffff;
}

/* generate digits from hs (hs must be at least 20 bytes long)
 * digits can be 1 .. 9 */
uint32_t hs2otp(unsigned char *hs,int digits)
{
	uint32_t bc;		/* bin code (dword) */
	uint32_t mdl=1;		/* 10^digits used for modulo */
	if(digits<1 || digits>9)
	{
		fprintf(stderr,"otp: wrong digits value!\n");
		return -1;
	}
	bc=hstrunc(hs);
#ifdef DEBUG
	printf("bc=%x\n",bc);
#endif
	while(digits)
	{
		mdl*=10;
		digits--;
	}
	return bc%mdl;
}

/* HMAC wrapper
 * hash (h) is allocated here, caller must free */
int my_hmac(unsigned char *k, unsigned int kl, unsigned char *m, unsigned int ml, unsigned char **h, unsigned int *hl, char *cypher)
{
	
	unsigned char *hs;
	int rv=0;

#ifdef HAVE_OPENSSL
	const EVP_MD *md;
	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname(cypher);
	if(md == NULL)
	{
		fprintf(stderr,"my_hmac (OpenSSL): Cypher %s not found\n",cypher);
		rv=1;
		goto eofunc_ssl;
	}
	hs=malloc(EVP_MD_size(md));
	if(hs == NULL)
	{
		fprintf(stderr,"my_hmac (OpenSSL): malloc error\n");
		rv=1;
		goto eofunc_ssl;
	}
	HMAC(md,k,kl,m,ml,hs,hl);
	*h=hs;
eofunc_ssl:
	EVP_cleanup();
#elif defined(USE_OWN_SHA1)
	hs=malloc(20);	/* since we only use SHA1 for now... */
	if(hs == NULL)
	{
		fprintf(stderr,"my_hmac (OpenSSL): malloc error\n");
		rv=1;
		goto eofunc_mysha1;
	}
	my_hmac_sha1(k,kl,m,ml,hs);
	if(hl) *hl=20;
	*h=hs;
eofunc_mysha1:
#elif defined(HAVE_WINCRYPT)
# error "Not yet implemented!"
	/* DO NOT USE! */
	/* TODO: adapt this example from MSDN, do not use */
	//--------------------------------------------------------------------
	// Declare variables.
	//
	// hProv:           Handle to a cryptographic service provider (CSP). 
	//                  This example retrieves the default provider for  
	//                  the PROV_RSA_FULL provider type.  
	// hHash:           Handle to the hash object needed to create a hash.
	// hKey:            Handle to a symmetric key. This example creates a 
	//                  key for the RC4 algorithm.
	// hHmacHash:       Handle to an HMAC hash.
	// pbHash:          Pointer to the hash.
	// dwDataLen:       Length, in bytes, of the hash.
	// Data1:           Password string used to create a symmetric key.
	// Data2:           Message string to be hashed.
	// HmacInfo:        Instance of an HMAC_INFO structure that contains 
	//                  information about the HMAC hash.
	// 
	HCRYPTPROV  hProv       = 0;
	HCRYPTHASH  hHash       = 0;
	HCRYPTKEY   hKey        = 0;
	HCRYPTHASH  hHmacHash   = 0;
	PBYTE       pbHash      = 0;
	DWORD       dwDataLen   = 0;
	BYTE        Data1[]     = {0x70,0x61,0x73,0x73,0x77,0x6F,0x72,0x64};
	BYTE        Data2[]     = {0x6D,0x65,0x73,0x73,0x61,0x67,0x65};
	HMAC_INFO   HmacInfo;

	//--------------------------------------------------------------------
	// Zero the HMAC_INFO structure and use the SHA1 algorithm for
	// hashing.

	ZeroMemory(&HmacInfo, sizeof(HmacInfo));
	HmacInfo.HashAlgid = CALG_SHA1;

	//--------------------------------------------------------------------
	// Acquire a handle to the default RSA cryptographic service provider.

	if (!CryptAcquireContext(
		&hProv,                   // handle of the CSP
		NULL,                     // key container name
		NULL,                     // CSP name
		PROV_RSA_FULL,            // provider type
		CRYPT_VERIFYCONTEXT))     // no key access is requested
	{
	   printf(" Error in AcquireContext 0x%08x \n",
			  GetLastError());
	   goto ErrorExit;
	}

	//--------------------------------------------------------------------
	// Derive a symmetric key from a hash object by performing the
	// following steps:
	//    1. Call CryptCreateHash to retrieve a handle to a hash object.
	//    2. Call CryptHashData to add a text string (password) to the 
	//       hash object.
	//    3. Call CryptDeriveKey to create the symmetric key from the
	//       hashed password derived in step 2.
	// You will use the key later to create an HMAC hash object. 

	if (!CryptCreateHash(
		hProv,                    // handle of the CSP
		CALG_SHA1,                // hash algorithm to use
		0,                        // hash key
		0,                        // reserved
		&hHash))                  // address of hash object handle
	{
		rv=-1;
/*	   printf("Error in CryptCreateHash 0x%08x \n",
			  GetLastError()); */
	   goto eofunc_wincrypt;
	}

	if (!CryptHashData(
		hHash,                    // handle of the hash object
		Data1,                    // password to hash
		sizeof(Data1),            // number of bytes of data to add
		0))                       // flags
	{
		rv=-2;
/*	   printf("Error in CryptHashData 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	if (!CryptDeriveKey(
		hProv,                    // handle of the CSP
		CALG_RC4,                 // algorithm ID
		hHash,                    // handle to the hash object
		0,                        // flags
		&hKey))                   // address of the key handle
	{
		rv=-3;
		/*
	   printf("Error in CryptDeriveKey 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	//--------------------------------------------------------------------
	// Create an HMAC by performing the following steps:
	//    1. Call CryptCreateHash to create a hash object and retrieve 
	//       a handle to it.
	//    2. Call CryptSetHashParam to set the instance of the HMAC_INFO 
	//       structure into the hash object.
	//    3. Call CryptHashData to compute a hash of the message.
	//    4. Call CryptGetHashParam to retrieve the size, in bytes, of
	//       the hash.
	//    5. Call malloc to allocate memory for the hash.
	//    6. Call CryptGetHashParam again to retrieve the HMAC hash.

	if (!CryptCreateHash(
		hProv,                    // handle of the CSP.
		CALG_HMAC,                // HMAC hash algorithm ID
		hKey,                     // key for the hash (see above)
		0,                        // reserved
		&hHmacHash))              // address of the hash handle
	{
		rv=-4;
	   /*printf("Error in CryptCreateHash 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	if (!CryptSetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HMAC_INFO,             // setting an HMAC_INFO object
		(BYTE*)&HmacInfo,         // the HMAC_INFO object
		0))                       // reserved
	{
		rv=-5;
		/*
	   printf("Error in CryptSetHashParam 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	if (!CryptHashData(
		hHmacHash,                // handle of the HMAC hash object
		Data2,                    // message to hash
		sizeof(Data2),            // number of bytes of data to add
		0))                       // flags
	{
		rv=-6;
		/*
	   printf("Error in CryptHashData 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	//--------------------------------------------------------------------
	// Call CryptGetHashParam twice. Call it the first time to retrieve
	// the size, in bytes, of the hash. Allocate memory. Then call 
	// CryptGetHashParam again to retrieve the hash value.

	if (!CryptGetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HASHVAL,               // query on the hash value
		NULL,                     // filled on second call
		&dwDataLen,               // length, in bytes, of the hash
		0))
	{
		rv=-7;
		/*
	   printf("Error in CryptGetHashParam 0x%08x \n", 
			  GetLastError());*/
	   goto eofunc_wincrypt;
	}

	pbHash = (BYTE*)malloc(dwDataLen);
	if(NULL == pbHash) 
	{
		rv=-8;
		/*
	   printf("unable to allocate memory\n");*/
	   goto eofunc_wincrypt;
	}
		
	if (!CryptGetHashParam(
		hHmacHash,                 // handle of the HMAC hash object
		HP_HASHVAL,                // query on the hash value
		pbHash,                    // pointer to the HMAC hash value
		&dwDataLen,                // length, in bytes, of the hash
		0))
	{
		rv=-9;
		/*
	   printf("Error in CryptGetHashParam 0x%08x \n", GetLastError());*/
	   goto eofunc_wincrypt;
	}

	// Print the hash to the console.


	printf("The hash is:  ");
	for(DWORD i = 0 ; i < dwDataLen ; i++) 
	{
	   printf("%2.2x ",pbHash[i]);
	}
	printf("\n");

	// Free resources.
eofunc_wincrypt:
	if(hHmacHash)
		CryptDestroyHash(hHmacHash);
	if(hKey)
		CryptDestroyKey(hKey);
	if(hHash)
		CryptDestroyHash(hHash);    
	if(hProv)
		CryptReleaseContext(hProv, 0);
	if(pbHash)
		free(pbHash);
#endif
	return rv;
}


/* "QuadWord" (8 bytes) to OTP conversion 
 * dst = pointer to destination string (must be digits+1 bytes long)
 * t = "time code" (epoch/step or any other counter)
 * digits = number of digits for OTP (0<digits<10)
 * cyph = OpenSSL cypher name 
 * k, kl = key, key length */
int qw2otp(char *dst, uint64_t t, int digits, char *cyph, unsigned char *k, int kl)
{
	char fmt[]="%0_u";	/* some of it changed below for padding */
	unsigned char shepc[8];
	int rv=0;
	unsigned int hl;
	unsigned char *shres=NULL;

	/* TODO: sanity checks */

	qw2s_be(shepc,t);
	my_hmac(k,kl,shepc,8,&shres,&hl,"sha1");
	/* for debug - print HMAC */
#ifdef DEBUG
	int i; /* for debug */
	for(i=0;i<hl;i++)
	{
		printf("%02x",shres[i]);
	}
	printf("\n");
#endif
	if(hl<20)
	{
		fprintf(stderr,"qw2otp: Hash length to small (cypher %s, hl %d)\n",cyph,hl);
		rv=1;
		goto eofunc;
	}
	
	/* rather roundabout method of padding a string with variable number of 0s
	 * first construct a format string, then use sprintf with that */
	fmt[2]=0x30|(0x0f&digits);
	sprintf(dst,fmt,hs2otp(shres,digits));

eofunc:
	free(shres);
	return rv;
}
