/* "QuadWord" (8 bytes) to OTP conversion 
 * dst = pointer to destination string (must be digits+1 bytes long)
 * t = "time code" (epoch/step or any other counter)
 * digits = number of digits for OTP (0<digits<10)
 * cyph = OpenSSL cypher name (ignored if using OWN_SHA1)
 * k, kl = key, key length */
int qw2otp(char *dst, uint64_t t, int digits, char *cyph, unsigned char *k, int kl);
