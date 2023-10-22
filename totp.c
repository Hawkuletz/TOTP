/* experiment to check SHA1-OTP as per rfc4226  */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "b32.h"
#include "sha1_otp.h"

void qw2s_be(unsigned char *dst,uint64_t e);

int main(int argc, char *argv[])
{
	size_t kl;

	uint64_t epc;
	char *b32k;
	uint8_t *bink;
	char sotp[32];

	if(argc<2 || argc>3)
	{
		fprintf(stderr,"Use: %s <key> [epoch]\n",argv[0]);
		fprintf(stderr,"Key should be (at most) 20bytes (base32-encoded)\n");
		exit(EXIT_FAILURE);
	}

	if(argc==3)	/* this is only for testing! */
		epc=strtol(argv[2],NULL,0);
	else
		epc=time(NULL);

	epc=epc/30;

	/* key is base32 encoded */

	/* block below is used for string key */
	b32k=argv[1];

	b32_deca(&bink,b32k,strlen(b32k),&kl);

	qw2otp(sotp,epc,6,"sha1",bink,kl);
	printf("OTP (new): %s\n",sotp);
	return 0;
}
