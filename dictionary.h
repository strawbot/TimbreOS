#include "bktypes.h"

// Dictionary structure
typedef struct {
	char ** table;	// entries point to strings
    Cell * adjunct;// contains an associated value
    Short capacity; // maximum 64K entries
	Short free;		// how many left
} dictionary_t;

void dictInsert(char * string, dictionary_t * dict);
void dictAppend(char * string, dictionary_t * dict);
void dictDelete(char * string, dictionary_t * dict);
char * dictFind(char * string, dictionary_t * dict);
Cell * dictAdjunct(char * string, dictionary_t * dict);
dictionary_t * dictionary(Short size);
void initDict(dictionary_t * dict, Short n);
void emptyDict(dictionary_t * dict);
