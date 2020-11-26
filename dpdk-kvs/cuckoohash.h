#ifndef _CUCKOOHASH_H
#define _CUCKOOHASH_H

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>



#include "config.h"
#include "cuckoohash_config.h"
//#include "hash.h"
#include "city.h"
#include "util.h"


typedef enum {
    ok = 0,
    failure = 1,
    failure_key_not_found = 2,
    failure_key_duplicated = 3,
    failure_space_not_enough = 4,
    failure_function_not_supported = 5,
    failure_table_full = 6,
} cuckoo_status;


/*
 * the structure of a buckoo hash table
 */
typedef struct {

    /* number of items inserted */
    size_t hashitems;

    /* 2**hashpower is the number of buckets */
    size_t hashpower;

    /* the mask for bucket index */
    size_t hashmask;

    /* pointer to the array of buckets */
    void*  buckets;

    /*
     *  keyver_array is an array of version counters
     *  we keep keyver_count = 8192
     *
     */
    void* keyver_array;

    /* the mutex to serialize insert, delete, expand */
    pthread_mutex_t lock;

    /* record the path */
    void* cuckoo_path;

    /* number of cuckoo operations*/
    size_t kick_count;

} cuckoo_hashtable_t;



/** 
 * @brief Initialize the hash table
 * 
 * @param h handler to the hash table
 * @param hashtable_init The logarithm of the initial table size
 *
 * @return handler to the hashtable on success, NULL on failure
 */
cuckoo_hashtable_t* cuckoo_init(const int hashpower_init);

/** 
 * @brief Cleanup routine
 * 
 */
cuckoo_status cuckoo_exit(cuckoo_hashtable_t* h);


/** 
 * @brief Lookup key in the hash table
 * 
 * @param h handler to the hash table
 *
 * @param key key to search 
 * @param val value to return
 * 
 * @return ok if key is found, not_found otherwise
 */
cuckoo_status cuckoo_find(cuckoo_hashtable_t* h, const char *key, char *val);



/** 
 * @brief Insert key/value to cuckoo hash table
 * 
 *  Inserting new key/value pair. 
 *  If the key is already inserted, the new value will not be inserted
 *
 *
 * @param h handler to the hash table
 * @param key key to be inserted
 * @param val value to be inserted
 * 
 * @return ok if key/value are succesfully inserted
 */
cuckoo_status cuckoo_insert(cuckoo_hashtable_t* h, const char *key, const char* val);


/** 
 * @brief Delete key/value from cuckoo hash table
 * 
 * @param h handler to the hash table
 * @param key key to be deleted
 *
 * @return ok if key is succesfully deleted, not_found if the key is not present
 */
cuckoo_status cuckoo_delete(cuckoo_hashtable_t* h, const char *key);


/** 
 * @brief Print stats of this hash table
 * 
 * @param h handler to the hash table
 * 
 * @return Void
 */
void cuckoo_report(cuckoo_hashtable_t* h);

/**
 * @breif copy the value of key b to a
 *
 * @param a dest key
 * @param b source key
 *
 * @return void
 */
void    copy_key(void * a, void * b);

/**
 * @breif copy the value of val b to a
 *
 * @param a dest value
 * @param b source value
 *
 * @return void
 */
void    copy_value(void *a, void *b);

/**
 * @breif copy the value of an integer to the first 4 bytes of a value
 *
 * @param a destination value
 * @param b source integer
 *
 * @return void
 */
void int_to_val(ValType *a, int b);
/**
 * @breif copy the value of an integer to the first 4 bytes of a key
 *
 * @param a destination key
 * @param b source integer
 *
 * @return void
 */
void int_to_key(KeyType *a, int b);

/**
 * @breif compairs the first 4 bytes of a value to an integer for equality
 *
 * @param v value to be compaired
 * @param i the integer to be compaired
 *
 * @return 0 if not equal 1 if equal
 */
int eq_int_val(ValType v, int i);

/**
 * @breif compairs the first 4 bytes of a key to an integer for equality
 *
 * @param v value to be compaired
 * @param i the integer to be compaired
 *
 * @return 0 if not equal 1 if equal
 */
int eq_int_key(KeyType k, int i);

KeyType empty_key;
ValType empty_value;

#endif
