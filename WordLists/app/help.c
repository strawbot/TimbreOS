
#include <stdlib.h>
#include <string.h>
#include "printers.h"
#include "cli.h"

void help(void);
void printif(char *s);

static char *filter;

void printif(char *s)
{
	if (strstr(s, filter) != NULL)
		print(s);
}

void help(void) {
	cursorReturn();
	parse(0);
	here();
	filter = (char *)ret();
	filter[filter[0] + 1] = 0;
	filter++;
    printif("help   print words with one line help; allow wild card filtering\n");
    printif("hi   test funciton\n");
    printif("tx   ( c ) send a character out LEUART 1 port\n");
    printif("words   list all words in dictionary\n");
}
