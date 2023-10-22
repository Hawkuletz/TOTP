# Build my_sha1 (sha1sum), hmac_mysha1, hmac_openssl
CC = gcc
CCFLAGS = -Wall -O2

all: totp

mysha1:
	$(CC) $(CCFLAGS) -DHAVE_ENDIAN -c mysha1.c

hmac_mysha1:mysha1
	$(CC) $(CCFLAGS) hmac_mysha1.c mysha1.o -o hmac_mysha1

sha1_otp:
	$(CC) $(CCFLAGS) -DHAVE_ENDIAN -DUSE_OWN_SHA1 -c sha1_otp.c

b32:
	$(CC) $(CCFLAGS) -DHAVE_ENDIAN -c b32.c

totp:sha1_otp mysha1 b32
	$(CC) $(CCFLAGS) totp.c b32.o sha1_otp.o mysha1.o -o totp

clean:
	\rm -f totp *.o
