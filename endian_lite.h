/* endian "lite" - a few macros to help on systems wihtout endian.h */
#ifndef _ENDIAN_LITE_H
#define _ENDIAN_LITE_H

/* define simple byteswap32 and byteswap64 macros */
# ifndef _bswap32
#  define _bswap32(x) \
	((((x) & 0xff000000) >> 24 ) | \
	 (((x) & 0x00ff0000) >>  8 ) | \
	 (((x) & 0x0000ff00) <<  8 ) | \
	 (((x) & 0x000000ff) << 24 ))
# endif /* _bswap32 */
# ifndef _bswap64
#  define _bswap64(x) \
	((((x) & 0xff00000000000000) >> 56 ) | \
	 (((x) & 0x00ff000000000000) >> 40 ) | \
	 (((x) & 0x0000ff0000000000) >> 24 ) | \
	 (((x) & 0x000000ff00000000) >>  8 ) | \
	 (((x) & 0x00000000ff000000) <<  8 ) | \
	 (((x) & 0x0000000000ff0000) << 24 ) | \
	 (((x) & 0x000000000000ff00) << 40 ) | \
	 (((x) & 0x00000000000000ff) << 56 ))
# endif /* _bswap64 */
# if defined(BIG_ENDIAN_MACHINE)	/* not tested! */
#  define htobe32(x) (x)
#  define htobe64(x) (x)
#  define be32toh(x) (x)
#  define be64toh(x) (x)
# elif defined(LITTLE_ENDIAN_MACHINE)
#  define htobe32(x) _bswap32(x)
#  define htobe64(x) _bswap64(x)
#  define be32toh(x) _bswap32(x)
#  define be64toh(x) _bswap64(x)
# else
#  error "Please define one of LITTLE_ENDIAN_MACHINE or BIG_ENDIAN MACHINE"
# endif	/* BIG_ENDIAN_MACHINE | LITTLE_ENDIAN_MACHINE */

#endif	/* ENDIAN_LITE_H */
