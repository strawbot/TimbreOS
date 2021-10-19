#include "ttypes.h"

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

// Dictionary structure
typedef struct {
    char ** table;	// entries point to strings
    Cell * adjunct; // contains an associated value
    Short capacity; // maximum 64K entries
    Short free;		// how many left
    bool upsize;    // whether to upsize or empty the dictionary when full
    Short iter;     // used for iterating over dictionary
} dictionary_t;

dictionary_t * dictionary(Short size);
void initDict(dictionary_t * dict, Short n);
void freeDict(dictionary_t * dict);
void emptyDict(dictionary_t * dict);
void upsizeDict(dictionary_t * dict);

Cell * dictInsert(char * string, dictionary_t * dict);
void dictAppend(char * string, dictionary_t * dict);
void dictAddKey(Cell address, dictionary_t * dict);
void dictDelete(char * string, dictionary_t * dict);
char * dictFind(char * string, dictionary_t * dict);
Cell dictFindKey(Cell address, dictionary_t * dict);
Cell * dictAdjunct(char * string, dictionary_t * dict);
Cell * dictAdjunctKey(Cell address, dictionary_t * dict);

char * dictFirst(dictionary_t * dict);
char * dictNext(dictionary_t * dict);

// prime numbers just less than 2**n
enum {
    HASH8 =(1<<8) -5,
    HASH9 =(1<<9) -3,
    HASH10=(1<<10)-3,
    HASH11=(1<<11)-9,
    HASH12=(1<<12)-3,
    HASH13=(1<<13)-1,
    HASH14=(1<<14)-3,
    HASH15=(1<<15)-19,
    HASH16=(1<<16)-15
};

// create static dictionary
#define HASHDICT(n, name) \
    static char * name##table[n]={NULL}; \
    static Cell name##adjunct[n]={0}; \
	dictionary_t name={&name##table[0], &name##adjunct[0], n, n/2, false, 0}

#endif
