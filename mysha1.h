#include <inttypes.h>

typedef struct my_sha1_wd
{
	uint64_t	ml;			/* message length in bits */
	uint32_t	h[5];		/* working hash */
	uint8_t		cc[320];	/* current block */
	uint32_t	cpos;		/* current index (in block) */
} my_sha1_wd;	/* working data */

/* initialize working structure */
void my_sha1_init(my_sha1_wd *wd);

/* process data block - returns 1 if maximum length exceeded, 0 otherwise */
int my_sha1_addblock(my_sha1_wd *wd, uint8_t *d, uint32_t bl);

/* compute final hash and places it in result */
int my_sha1_finalize(my_sha1_wd *wd, uint8_t *result);

/* convenient wrapper for calculating hash of a single data block */
void my_sha1(uint8_t *m, uint32_t bl, uint8_t *md);

/* generate hmac using sha1 */
void my_hmac_sha1(uint8_t *k, uint32_t kl, uint8_t *m, uint32_t ml, uint8_t *h);

