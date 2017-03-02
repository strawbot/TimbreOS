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
#include "printers.h"
#include <string.h>
#include <stdlib.h>

// Helper
static void * allocate(size_t size)
{
    void * m = malloc(size);

    if (m == NULL)
        print("\nMemory Allocation Failed!");

    return m;
}

static void qfree(Cell address)
{
    if (address)
        free((void *)address);
}

#define HASH_SEED 8315	// hash starting point

// hash table sizes
static Short primeSizes[] = {HASH8, HASH9, HASH10, HASH11, HASH12, HASH13, HASH14, HASH15, HASH16};

static Short hashSize(Short n)  // select table size
{
    for (Long i = 0; i < sizeof(primeSizes)/sizeof(primeSizes[0]); i++)
        if (primeSizes[i]/2 >= n)
            return primeSizes[i];
    return primeSizes[sizeof(primeSizes)/sizeof(primeSizes[0])-1];
}

// Table checks
static char zeroString[] = {'\0'}; // used in place of deleted locations

static bool used(char * string) // true if string is valid and not zerostring
{
    if (string)
        return *string != '\0';
    return false;
}

static bool same(char * s1, char * s2) // true if string matches
{
    if (s1 == s2)
        return true;
    if (*s2 == 0)
        return false;
    return 0 == strcmp(s1, s2);
}

static bool different(char *string, char ** loc) // true if string valid and different
{
    if (loc == NULL)
        return false;
    if (*loc == NULL)
        return false;
    return !same(string, *loc);
}

// Hashing algorithms
static Short hash(char * string, dictionary_t * dict)
{
    Short hash = HASH_SEED;

    while (*string) {
        hash ^= (Short)(*string++);
        hash = (hash << 3) | (hash >> 13);
    }

    hash %= dict->capacity;
    return hash;
}

// rehash based on first character of string
static Short rehash(char * string, Short index, dictionary_t * dict)
{
    index += (*string + 1);
    index %= dict->capacity;

    return index;
}

// locate start or end of chain of same string
static Short locate(char * string, dictionary_t * dict)
{
    Short index = hash(string, dict);

    while(different(string, &dict->table[index]))
        index = rehash(string, index, dict);

    return index;
}

static Short locateAppend(char * string, dictionary_t * dict)
{
    Short index = hash(string, dict);

    while(used(dict->table[index]))
        index = rehash(string, index, dict);

    return index;
}

// growth and shrink
static void plusEntry(dictionary_t * dict) // upsize if full and allowed; otherwise empty
{
    if (dict->free == 0) {
        if (dict->upsize)
            upsizeDict(dict);
        else
            emptyDict(dict);
    }
    dict->free -= 1;
}

static void minusEntry(dictionary_t * dict) // place to downsize if desired
{
    dict->free += 1;
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
    dict->adjunct = (Cell *)allocate(dict->capacity*sizeof(Cell));
    emptyDict(dict);
}

void setUpsize(bool flag, dictionary_t * dict)
{
    dict->upsize = flag;
}

void emptyDict(dictionary_t * dict) // empty out any content
{
    for (Short i=0; i<dict->capacity; i++) {
        dict->table[i] = NULL;
        dict->adjunct[i] = 0;
    }
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
    Short index, last;
    dictionary_t old;

    old.adjunct = dict->adjunct;
    old.capacity = dict->capacity;
    old.free = dict->free;
    old.table = dict->table;
    old.upsize = dict->upsize;

    initDict(dict, old.capacity);               // get a new dictionary

    for (Short i=0; i<old.capacity; i++) {
        char * string = old.table[i];

        if (used(string)) {
            while (true) { // relocate all strings that match
                index = locate(string, &old); // get first occurance in old dict
                string = old.table[index];

                if (!used(string)) break;

                last = locateAppend(string, dict); // append to new dict
                dict->table[last] = string;
                dict->adjunct[last] = old.adjunct[index];

                old.table[index] = zeroString;      // remove from old dict
            }
        }
    }
    freeDict(&old);
}

// Usage
void dictInsert(char * string, dictionary_t * dict) // insert a string into the dictionary
{
    Cell adjunct = 0, temp;
    Short index;

    plusEntry(dict);
    index = hash(string, dict);
    while (used(dict->table[index])) {
        // if same, insert newer and push other deeper in the chain
        if (same(string, dict->table[index])) {
            char *s = dict->table[index];
            temp = dict->adjunct[index];

            dict->table[index] = string;
            dict->adjunct[index] = adjunct;

            string = s;
            adjunct = temp;
         }
         index = rehash(string, index, dict);
    }
    dict->table[index] = string;
    dict->adjunct[index] = adjunct;
}

void dictAppend(char * string, dictionary_t * dict) // append a string to the dictionary
{
    plusEntry(dict);
    dict->table[locateAppend(string, dict)] = string;
}

void dictDelete(char * string, dictionary_t * dict) // delete inserted string from dictionary
{
    Short index = locate(string, dict);

    if (dict->table[index]) {
        if (*dict->table[index]) {
            dict->table[index] = zeroString;
            minusEntry(dict);
            dict->adjunct[index] = 0;
        }
    }
}

char * dictFind(char * string, dictionary_t * dict) // find a string in the dict
{
    return dict->table[locate(string, dict)];
}

Cell * dictAdjunct(char * string, dictionary_t * dict) // return an associate cell for string
{
    Short index = locate(string, dict);

    if (!used(dict->table[index]))
        return NULL;

    return &dict->adjunct[index];
}

void dictPrint(dictionary_t *dict)
{
	for (char * entry = dictFirst(dict); entry; entry = dictNext(dict)) {
		print("\n"), print(entry);
		print("  adjunct: "), printHex(dict->adjunct[dict->iter]);
    }
}

// Iterator access
// must use dictFirst to initiate; then dictNext to get subsequent entries
// end is detected when NULL string is returned from either function
char * dictFirst(dictionary_t * dict)
{
	dict->iter = 0;
	if (used(dict->table[0]))
		return dict->table[0];
	else
		return dictNext(dict);
}

char * dictNext(dictionary_t * dict)
{
	Short i = dict->iter;

    while (++i < dict->capacity)
        if (used(dict->table[i])) {
        	dict->iter = i;
        	return dict->table[i];
        }
    return NULL;
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

 8. The assumption to be tested is that link lists of the same string will always maintain order
    and not get out of order as the dicitonary fills up. This is the reason for zeroString as a
    place holder.
*/
