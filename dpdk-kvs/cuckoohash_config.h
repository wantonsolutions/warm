#ifndef _CUCKOOHASH_CONFIG_H
#define _CUCKOOHASH_CONFIG_H
#include <stdint.h>

#ifndef KEY_SIZE
#define KEY_SIZE 4
#endif

#ifndef VAL_SIZE
#define VAL_SIZE 4
#endif

typedef uint8_t KeyType[KEY_SIZE];
//typedef uint32_t ValType;
typedef uint8_t ValType[VAL_SIZE];

/* size of bulk cleaning */
#define DEFAULT_BULK_CLEAN 1024


/* set DEBUG to 1 to enable debug output */
#define DEBUG 1


#endif
