/*
 * 
 * taken from http://www.azillionmonkeys.com/qed/hash.html
 * 
 * */
/*#include "stdint.h" / * Replace with <stdint.h> if appropriate */
#include <esp8266.h>
#if 0
#define DBG(format, ...) do { os_printf(format, ## __VA_ARGS__); } while(0)
#else
#define DBG(format, ...) do { ; }while(0)
#endif
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((int32)(((const uint8 *)(d))[1])) << 8)\
                       +(int32)(((const uint8 *)(d))[0]) )
#endif

int32 SuperFastHash (const char * data) {
int32 len =0;
int32 hash, tmp;
int rem;
DBG("SFH: [%s]...\n",data);
while(len < 128 && data[len])len++;
hash=len;
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
	DBG("SFH: ->[%d]\n",hash);
    return hash;
}
