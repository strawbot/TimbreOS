// Dictionaries  Robert Chapman III  orig Apr 15, 91 - in C  Oct 27, 15
// fast hashed string lookup with adjunct meaning

/* Notes:
 *  use 1% of capacity for original free value
 *  when free goes to zero, check resize flag to see if it should be upsized
 *  if you want a dictionary of size == capacity, it will not fit because free is always < capacity
 *   should free be less than capacity?
 *    free = 99*capacity/100;
 *  Could add a marker to dictionary structure that indicates first use and whether it is valid or not
 *  to free up tables. This way initDict could be used without emptyDict.
 */
#include "dictionary.h"
#include <string.h>
#include <stdlib.h>

// Externals
// Helper
void * allocate(size_t size)
{
    void * m = malloc(size);

    if (m == NULL)
        error("Memory Allocation Failed!");

    return m;
}

static void qfree(Cell address)
{
	if (address)
        free((void *)address);
}

// growth and shrink
#define BUFFER 20		// keep this much left
#define HASH_SEED 8315	// hash starting point
// prime numbers just less than n**2
static Short primeSizes[] = {256-5,512-3,1024-3,2048-9,4096-3,8192-1,16384-3,32768-19,65536-15};

static Short hashSize(Short n)  // select table size
{
    for (Long i = 0; i < sizeof(primeSizes); i++)
		if (primeSizes[i] >= n)
			return primeSizes[i];
    return primeSizes[sizeof(primeSizes)/sizeof(primeSizes[0])-1];
}

static void plusEntry(dictionary_t * dict) // place to upsize; could use 2/3 full crit to upsize
{
	if (dict->free == 0) {
		note("Dictionary full. Emptying.");
		emptyDict(dict);
        initDict(dict, dict->capacity);
	}
	dict->free -= 1;
}

static void minusEntry(dictionary_t * dict) // place to downsize if desired
{
	dict->free += 1;
}

static void checkAdjunct(dictionary_t * dict)
{
    if (dict->adjunct == 0) {
        dict->adjunct = (Cell *)allocate(dict->capacity*sizeof(Cell));
        memset(dict->adjunct, 0, dict->capacity*sizeof(Cell));
    }
}

// Table checks
static char zeroString[] = {'\0'}; // used to fill previously filled locations

static bool used(char * string)
{
	if (string)
        return *string != '\0';
    return false;
}

static bool same(char * s1, char * s2)
{
    if (s1 == s2)
        return true;
    if (*s2 == 0)
        return false;
	return 0 == strcmp(s1, s2);
}

static bool different(char *string, char ** loc)
{
    if (loc == NULL)
        return false;
    if (*loc == NULL)
        return false;
    return !same(string, *loc);
}

// Hashing algorithms
static char ** hash(char * string, dictionary_t * dict)
{
	Short hash = HASH_SEED;
	
	while (*string) {
		hash ^= (Short)(*string++);
		hash = (hash << 3) | (hash >> 13);
	}

    hash %= dict->capacity;
    return &dict->table[hash];
}

// rehash based on first character of string
static char ** rehash(char * string, char  ** loc, dictionary_t * dict)
{
	loc += (*string + 1) * sizeof(Cell);
    if (loc >= &dict->table[dict->capacity - 1]) {
        Short index = loc - &dict->table[0];

        index %= dict->capacity;
        loc = &dict->table[index];
    }
    return loc;
}

// locate and bump if necessary
static char ** locate(char * string, dictionary_t * dict)
{
    char ** loc  = hash(string, dict);

    while (different(string, loc))
        loc = rehash(string, loc, dict);

	return loc;
}

static char * bump(char * string, char ** loc)
{
	char * bumped = *loc;
	
	*loc = string;
	return bumped;
}

static Cell bumpAdjunct(Cell adjunct, char ** loc, dictionary_t * dict)
{
	Short index;
	Cell bumped;
	
	if (dict->adjunct == NULL) return adjunct;
	
    index = (Short)(loc - dict->table)/sizeof(Cell);
	bumped = dict->adjunct[index];
	dict->adjunct[index] = adjunct;
	
	return bumped;
}

static void deleteAdjunct(char ** loc, dictionary_t * dict)
{
	if (dict->adjunct)
        dict->adjunct[(Short)(loc - dict->table)/sizeof(Cell)] = 0;
}

// Dictionary interface
// hash to location yields 3 results:
//  if empty use it
//  else if occupied by same string, bump string along then rehash
//  else rehash
void dictInsert(char * string, dictionary_t * dict) // insert a string into the dictionary
{
	Cell adjunct = 0;
	char ** loc;

	plusEntry(dict);
	loc = hash(string, dict);
    while (used(*loc)) {
        if (same(string, *loc)) {
			string = bump(string, loc);
			adjunct = bumpAdjunct(adjunct, loc, dict);
		}
		loc = rehash(string, loc, dict);
	}
	*loc = string;
    if (dict->adjunct != NULL)
        dict->adjunct[(Short)(loc - dict->table)/sizeof(Cell)] = adjunct;
}

void dictAppend(char * string, dictionary_t * dict) // append a string to the dictionary
{
	char ** loc;

	plusEntry(dict);
	loc = hash(string, dict);
    while (used(*loc))
		loc = rehash(string, loc, dict);
	*loc = string;
}
	
void dictDelete(char * string, dictionary_t * dict) // delete inserted string from dictionary
{
    char ** loc = locate(string, dict);

    if (*loc) {
        if (**loc) {
            *loc = &zeroString[0];
            minusEntry(dict);
            deleteAdjunct(loc, dict);
		}
	}
}

char * dictFind(char * string, dictionary_t * dict) // find a string in the dict
{
    return *locate(string, dict);
}

Cell * dictAdjunct(char * string, dictionary_t * dict) // return an associate cell for string
{
	char ** loc  = locate(string, dict);

	checkAdjunct(dict);
    return &dict->adjunct[(loc - dict->table)/sizeof(Cell)];
}

dictionary_t * dictionary(Short size) // return a dictionary big enough to hold size
{
    dictionary_t * dict = (dictionary_t *)allocate(sizeof(dictionary_t));
	
    initDict(dict, size);
	
	return dict;
}

void initDict(dictionary_t * dict, Short n) // fill in dictionary template and allocate string table
{
	Short size = hashSize(n);
    char ** table = (char**)allocate(size * sizeof(char**));

	dict->capacity = size;
	dict->free = size - BUFFER;
    dict->table = (char **)table;
    for (Short i=0; i<dict->capacity; i++)
        dict->table[i] = NULL;
    dict->adjunct = NULL;
}

void emptyDict(dictionary_t * dict) // return previous tables and start anew
{
	qfree((Cell)dict->table);
	qfree((Cell)dict->adjunct);
    dict->table = NULL;
    dict->adjunct = NULL;
}


/* Notes:
 1. to upsize: get dictionary size and double it.
 walk through 1st dictionary until all locations unused. For each location,
 get string, do a find, then move string and adjunct to new dictionary. This reverses
 the order of same strings tho. Perhaps that would need to be recursive to do it right.
 Recognizing same strings as a special case could be sped up for upsizing.
 
 2. start with a statically generated dictionary so it can be used withouth memory
 allocation? Or check at compile to see if memory allocation is available and if not
 do a static allocation? or leave that as outside the scope of the tool.
 
 3. empty a dictionary out by calling empty and then either call initialize with a new
 size to start afresh or do a free on the dictionary structure. The size could also be
 passed into empty as a parameter.
 
 4. should dictionary structure be linked to previous one?

 5. purpose of zeroString is to hold a location in a chain of rehashes. It can be replaced but who
 will signal its replacement? When will it be reused? Append and Insert should be able to use it.

 6. to keep collisions low the free is much less than capacity. Perhaps half.
    Once there are no more free, then a new dictionary twice the size should
    be generated. When requesting a size, the dict size returned should be at
    least twice the size. I.E. request 240 means that it must fit at least
    480. Capacity will be 509 and free will be 254.

*/
