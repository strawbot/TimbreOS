// Dictionaries  Robert Chapman III  orig Apr 15, 91 - in C  Oct 27, 15
// fast hashed string lookup with adjunct meaning; key,value

/* Notes:
 *  There are 3 types of hashed dictionaries:
    a. totally static; must only manage with emptyDict
       HASHDICT(HASH8, name);
    b. static structure, dynamic tables; use emptyDict, freeDict, initDict, upsizeDict
       static dictionary_t name;
       initDict(&name, n);
    c. all dynamic; use emptyDict, freeDict, initDict, upsizeDict and free when done
       dictionary_t name = dictionary(n);
 */

#include "dictionary.h"
#include <string.h>
#include <stdlib.h>

// Helper
static void * allocate(size_t size)
{
    void * m = malloc(size);

    if (m == NULL)
        errorMsg("Memory Allocation Failed!");

    return m;
}

static void qfree(Cell address)
{
	if (address)
        free((void *)address);
}

// growth and shrink
#define HASH_SEED 8315	// hash starting point
// hash table sizes
static Short primeSizes[] = {HASH8, HASH9, HASH10, HASH11, HASH12, HASH13, HASH14, HASH15, HASH16};

static Short hashSize(Short n)  // select table size
{
    for (Long i = 0; i < sizeof(primeSizes); i++)
        if (primeSizes[i]/2 >= n)
			return primeSizes[i];
    return primeSizes[sizeof(primeSizes)/sizeof(primeSizes[0])-1];
}

static void plusEntry(dictionary_t * dict) // upsize if full and allowed; otherwise empty
{
	if (dict->free == 0) {
        if (dict->upsize)
            upsizeDict(dict);
        else {
            emptyDict(dict);
        }
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
// hash to location yields 3 results:
//  if empty use it
//  else if occupied by same string, bump string along then rehash
//  else rehash
static char ** locate(char * string, dictionary_t * dict)
{
    char ** loc  = hash(string, dict);

    while (different(string, loc))
        loc = rehash(string, loc, dict);
	return loc;
}

static char ** locateAppend(char * string, dictionary_t * dict)
{
    char ** loc = hash(string, dict);

    while (used(*loc))
        loc = rehash(string, loc, dict);
    return loc;
}

static char ** locateLast(char * string, dictionary_t * dict)
{
    char ** loc, ** matchloc;

    loc = matchloc = hash(string, dict);

    while (true) {

        if (used(*loc)) {
            if (same(string, *loc))
                matchloc = loc;
            loc = rehash(string, loc, dict);
        }
        else
            break;
    }
    return matchloc;
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

static Cell * adjunctLocation(char ** loc, dictionary_t * dict)
{
    return &dict->adjunct[(loc - dict->table)/sizeof(Cell)];
}

static void deleteAdjunct(char ** loc, dictionary_t * dict)
{
	if (dict->adjunct)
        *adjunctLocation(loc, dict) = 0;
}

// Dictionary interface

// Management
dictionary_t * dictionary(Short size) // return a dictionary big enough to hold size
{
    dictionary_t * dict = (dictionary_t *)allocate(sizeof(dictionary_t));
	
    initDict(dict, size);
	
	return dict;
}

void initDict(dictionary_t * dict, Short n) // fill in dictionary template and allocate string table
{
    dict->capacity = hashSize(n);
    dict->table = (char**)allocate(dict->capacity * sizeof(char**));
    dict->adjunct = NULL;
    emptyDict(dict);
}

void setUpsize(bool flag, dictionary_t * dict)
{
    dict->upsize = flag;
}

void emptyDict(dictionary_t * dict) // empty out any content
{
    for (Short i=0; i<dict->capacity; i++)
        dict->table[i] = NULL;
    if (dict->adjunct)
        memset(dict->adjunct, 0, dict->capacity*sizeof(Cell));
    dict->free = dict->capacity/2;
    dict->upsize = false;
}

void freeDict(dictionary_t * dict) // return previous tables and start anew
{
	qfree((Cell)dict->table);
	qfree((Cell)dict->adjunct);
    dict->table = NULL;
    dict->adjunct = NULL;
}

// Upsizing dictionary
// must maintain ordering of strings and their associated ajunct value
// first one found in old is deleted from old and appended to new; continue till string is not in old
void upsizeDict(dictionary_t * dict)
{
    dictionary_t old;

    old.adjunct = dict->adjunct;
    old.capacity = dict->capacity;
    old.free = dict->free;
    old.table = dict->table;
    old.upsize = dict->upsize;

    initDict(dict, old.capacity);               // get a new dictionary
    if (old.adjunct)
        checkAdjunct(dict);

    for (Short i=0; i<old.capacity; i++) {
        char * string = old.table[i];
        char ** locold;
        Cell adj;

        if (used(string)) {
            while (true) {
                locold = locate(string, &old);
                if (used(*locold)) {
                    dictAppend(*locold, dict);
                    if (old.adjunct) {
                        adj = *adjunctLocation(locold, &old);
                        if (adj)
                            *adjunctLocation(locateLast(string, dict), dict) = adj;
                    }
                    *locold = zeroString;
                }
                else
                    break;
            }
        }
    }
    freeDict(&old);
}

// Usage
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
        *adjunctLocation(loc, dict) = adjunct;
}

void dictAppend(char * string, dictionary_t * dict) // append a string to the dictionary
{
    plusEntry(dict);
    *locateAppend(string, dict) = string;
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

    if (!used(*loc))
        return NULL;

    checkAdjunct(dict);
    return adjunctLocation(loc, dict);
}

/* Notes:
 1. What to do if allocation of memory fails?

 2. purpose of zeroString is to hold a location in a chain of rehashes. It can be replaced but who
    will signal its replacement? When will it be reused? Append and Insert should be able to use it.

 3. since used locations don't get passed to new dictionary when upsizing, there might be a
    different dynamic. But it might be ok. Kind of clears out the clutter. Only there if deletes are used.

 4. statically defined dictionaries could not be upgraded and should contain a flag to prevent that. Perhaps
    the upsize flag would prevent initdict from doing a free and instead would force it to empty all locations.
    Maybe there is a cleanDict() function which only does the cleaning or table and adjunct and resetting
    of free.

 7. should the HASHn sized reflect the free size and not the capacity?
*/
