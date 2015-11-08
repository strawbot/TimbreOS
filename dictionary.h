#include "bktypes.h"

// Dictionary structure
typedef struct {
	char ** table;	// entries point to strings
    Cell * adjunct; // contains an associated value
    Short capacity; // maximum 64K entries
	Short free;		// how many left
    bool upsize;    // whether to upsize or empty the dictionary when full
} dictionary_t;

void dictInsert(char * string, dictionary_t * dict);
void dictAppend(char * string, dictionary_t * dict);
void dictDelete(char * string, dictionary_t * dict);
char * dictFind(char * string, dictionary_t * dict);
Cell * dictAdjunct(char * string, dictionary_t * dict);
dictionary_t * dictionary(Short size);
void initDict(dictionary_t * dict, Short n);
void emptyDict(dictionary_t * dict);
void upsizeDict(dictionary_t * dict);
Cell * adjunctLocation(char ** loc, dictionary_t * dict);
char ** locateAppend(char * string, dictionary_t * dict);
char ** locateLast(char * string, dictionary_t * dict);

// Static definition of hash tables with no adjunct
enum {// prime numbers just less than n**2
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

// need a way to initialize the embedded arrays tho
/* attempt 1
#define DICT_TABLE(n, name)  dictionary_t name = {Cell [n], NULL, n, n/2, false}
#define DICT_ADJUNCT(n, name)  dictionary_t name = {Cell [n], Cell [n], n, n/2, false}
*/
/* attempt 2
#define DICT_TABLE(n, name) struct name{Cell table[n]; Cell adjunct[n]; Short capacity; Short free; bool upsize;} name = {0,0,n,n/2,false}
*/
// attempt 3; like QUEUE, BYTEQ
#define HASHDICT(n, name) \
    static char * name##table[n]={NULL}; \
    static Cell name##adjunct[n]={0}; \
    dictionary_t name={&name##table[0], &name##adjunct[0], n, n/2, false}

/* would like it to work but not allowed
#define DICT(n, name) \
#if   n/2 < HASH8  DICT_TABLE(HASH8, name) \
#elif (n/2 < HASH9)  DICT_TABLE(HASH9, name) \
#elif (n/2 < HASH10) DICT_TABLE(HASH10, name) \
#elif (n/2 < HASH11) DICT_TABLE(HASH11, name) \
#elif (n/2 < HASH12) DICT_TABLE(HASH12, name) \
#elif (n/2 < HASH13) DICT_TABLE(HASH13, name) \
#elif (n/2 < HASH14) DICT_TABLE(HASH14, name) \
#elif (n/2 < HASH15) DICT_TABLE(HASH15, name) \
#elif (n/2 < HASH16) DICT_TABLE(HASH16, name) \
#endif
*/
