#include "bktypes.h"

// Dictionary structure
typedef struct {
	char ** table;	// entries point to strings
    Cell * adjunct; // contains an associated value
    Short capacity; // maximum 64K entries
	Short free;		// how many left
    bool upsize;    // whether to upsize or empty the dictionary when full
} dictionary_t;

dictionary_t * dictionary(Short size);
void initDict(dictionary_t * dict, Short n);
void freeDict(dictionary_t * dict);
void emptyDict(dictionary_t * dict);
void upsizeDict(dictionary_t * dict);

void dictInsert(char * string, dictionary_t * dict);
void dictAppend(char * string, dictionary_t * dict);
void dictDelete(char * string, dictionary_t * dict);
char * dictFind(char * string, dictionary_t * dict);
Cell * dictAdjunct(char * string, dictionary_t * dict);

// prime numbers just less than n**2
enum {
    HASH8=256-5,
    HASH9=512-3,
    HASH10=1024-3,
    HASH11=2048-9,
    HASH12=4096-3,
    HASH13=8192-1,
    HASH14=16384-3,
    HASH15=32768-19,
    HASH16=65536-15
};

// create static dictionary
#define HASHDICT(n, name) \
    static char * name##table[n]={NULL}; \
    static Cell name##adjunct[n]={0}; \
    dictionary_t name={&name##table[0], &name##adjunct[0], n, n/2, false}
