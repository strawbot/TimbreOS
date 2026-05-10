#include <stdlib.h>
#include <string.h>
#include "printers.h"
#include "cli.h"
#include <ctype.h>

bool visible_word(char *s);

static char *filter;

const char *strcasestr_r(const char *haystack, const char *needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0) return haystack;
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0) return haystack;
    }
    return NULL;
}

static void printif(char *s) {
	if (strcasestr_r(s, filter) != NULL && visible_word(s))
		print(s);
}

static void helphelp() {
	print("Print commands with one line help with wild card filtering: help <filter>");
    print("\nCommands with Arguments");
    print("\n Some arguments precede the command while others have arguments after.");
    print("\n Generally, numbers come before while strings come after. A number as");
    print("\n a string will come after such as the commands setting port addresses");
    print("\n or ip addresses.");
    print("\n\n  Use:");
    print("\n   <s> for strings following a command");
    print("\n   (n) for parameters preceding a command");
    print("\n       if there are results they are preceded by -");
    print("\n       so ( a - n ) uses a and returns n");
    print("\n  For example:");
    print("\n    	rm   <pattern> remove files whose name matches pattern");
    print("\n      seek   ( n )  seek to position n in the file; -1 to end");
    print("\n        s!   ( h a - ) store next into memory using top as address (16 bit)");
    print("\n    negate   ( n - -n ) two's complement of top data stack item");
    print("\n\n  Command examples:");
    print("\n   rm somefilename");
    print("\n   124 seek");
    print("\n   1001 portaddress s!");
    print("\n   100 negate");
}

void help(void) {
	cursorReturn();
	parse(0);
	here();
	filter = (char *)ret()+1;
	if (strcmp("help", filter) == 0) {
		helphelp();
		return;
	}
    printif("!   ( n a - ) store next into memory using top as address (processor sized)\n");
    printif("#   ( n - n' ) convert a digit from n\n");
    printif("#>   ( n - a c ) finish number sequence and return address and count\n");
    printif("#s   ( n - 0 ) convert all digits in n\n");
    printif("(  [i]  start of comment till end of line or )\n");
    printif("*   ( n m - p ) multiply next data stack item by top and leave on top\n");
    printif("+   ( n m - p ) add top two data stack items and leave on top\n");
    printif("+b   ( b a - ) turn on b bits at address a: 0b10001 em +b\n");
    printif(",   ( n -s ) allocate 1 cell and put n into it\n");
    printif("-   ( n m - p ) subtract top data stack item from next item and leave on top\n");
    printif("-b   ( b a - ) turn off b bits at address a: 0b10001 em -b\n");
    printif(".   ( n - ) print n in current number base\n");
    printif(".b   ( n - ) print number in binary\n");
    printif(".d   ( n - ) print number in decimal\n");
    printif(".h   ( n - ) print number in hex\n");
    printif(".r   ( m n - ) print m in right field of n digits\n");
    printif(".s   print number of items on data stack and items\n");
    printif("/   ( n m - q ) divide next data stack item by top and leave on top\n");
    printif("/mod   ( n m - q r ) return divide and modulus from top item into next item\n");
    printif(":   <string> start a macro definition named string\n");
    printif(";  [i]  end a macro build\n");
    printif("<   ( n m - f ) leave a boolean on stack indicating if next is less than top\n");
    printif("<#   initiate a number sequence\n");
    printif("=   ( n m - f ) leave a boolean on stack after equating top two data stack items\n");
    printif(">   ( n m - f ) leave a boolean on stack indicating if next is greater than top\n");
    printif(">r   ( n - ) (R - n ) push the top item of the data stack onto the return stack\n");
    printif("?dup   ( n - n n | - 0 ) duplicate top data stack item if not 0\n");
    printif("@   ( a - n ) return contents of memory using top stack item as the address (processor sized)\n");
    printif("[  [i]  exit macro build\n");
    printif("]   enter macro build\n");
    printif("abs   ( n - |n|) top data stack item is made positive\n");
    printif("again  [i]  end of a continuous loop construct\n");
    printif("allot   ( n - ) reserve n bytes after end of dictionary\n");
    printif("and   ( n m - p ) bitwise AND top two data stack items and leave on top\n");
    printif("begin  [i]  start of a loop construct\n");
    printif("bin   switch to binary numbers\n");
    printif("c!   ( c a - ) store next into memory using top as address (8 bit)\n");
    printif("c,   ( c - ) allocate and 1 byte and put value in it\n");
    printif("c@   ( a - c ) return contents of memory using top stack item as the address (8 bit)\n");
    printif("cmove   ( s d n - ) move n bytes from s to d\n");
    printif("constant   ( n - ) <string> give n a name\n");
    printif("cr   send end of line to output device\n");
    printif("decimal   interpret all subsequent numbers as decimal\n");
    printif("drop   ( n - ) throw away the top data stack item\n");
    printif("dump   ( a n - ) dump n 16-byte rows of memory starting at address a\n");
    printif("dup   ( n - n n ) make a copy of the top data stack item\n");
    printif("else  [i]  otherwise part of an if statement\n");
    printif("emit   ( c - ) send c to output device\n");
    printif("endif  [i]  end of else or if statement\n");
    printif("erase   ( s n - ) erase n bytes from s\n");
    printif("execute   ( a - ) use the top data stack item as a function call\n");
    printif("exit  [i]  exit macro\n");
    printif("fill   ( s n x - )fill n bytes from s with x\n");
    printif("for  [i]  ( n - ) start of a loop which runs n times\n");
    printif("help   <filtering> print words with one line help; allow wild card <filtering>; parenthesis show ( args - results ) and precede the command; angle brackets show arguments that follow commands\n");
    printif("here   ( - a ) return address of end of dictionary\n");
    printif("hex   interpret all following numbers as hex\n");
    printif("hold   ( c - ) hold a character in number sequence\n");
    printif("if  [i]  ( n - ) execute following code if top of stack is non-zero\n");
    printif("max   ( n m - n|m) leave maximum of top two stack items\n");
    printif("min    ( n m - n|m) leave minimum of top two stack items\n");
    printif("mod   ( n m - r ) modulus next data stack item by top and leave on top\n");
    printif("negate   ( n - -n ) two's complement of top data stack item\n");
    printif("next  [i]  end of a for loop\n");
    printif("not   ( n - n' ) invert all bits on the top data stack item\n");
    printif("oct   switch to octal numbers\n");
    printif("or    ( n m - p ) bitwise OR top two data stack items and leave on top\n");
    printif("over   ( n m - n m n ) copy 2nd data stack item to top of data stack\n");
    printif("r   ( - n ) (R n - n ) copy the top item of the return stack onto the data stack\n");
    printif("r>    ( - n ) (R n - ) move top item on return stack to data stack\n");
    printif("repeat  [i]  go back to the begin part\n");
    printif("resetcli   reset cli including removing all macros\n");
    printif("s!   ( h a - ) store next into memory using top as address (16 bit)\n");
    printif("s@   ( a - h ) return contents of memory using top stack item as the address (16 bit)\n");
    printif("shift   ( n m - p ) shift n by m bit left for minus and right for positive\n");
    printif("sign   ( m n - n ) prepend sign to number sequence if m is negative\n");
    printif("sp!   ( ... - ) empty the data stack\n");
    printif("strlen   ( a - c ) return length of a string\n");
    printif("swap   ( n m - m n ) swap top two items on the data stack\n");
    printif("type   ( a n - ) output n characters starting at a\n");
    printif("until  [i]  ( n - ) go back to the begin statement if stack is zero\n");
    printif("variable   ( n - ) <string> give n a place to be stored at a name\n");
    printif("while  [i]  ( n - ) conditional choice in a loop construct\n");
    printif("words   list all words in dictionary\n");
    printif("xor   ( n m - p ) bitwise XOR top two data stack items and leave on top\n");
}
