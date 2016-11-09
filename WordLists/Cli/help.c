#include <stdlib.h>
#include "printers.h"
#include "botkernl.h"
#include "kernel.h"

void help(void);
void printif(char *s);

static char *filter;

void printif(char *s)
{
	if (strstr(s, filter) != NULL)
		print(s);
}

void help(void) {
	_CR();
	BL(), WORD(), HERE();
	filter = (char *)*sp++;
	filter[filter[0] + 1] = 0;
	filter++;
    printif("!   ( n a - ) store next into memory using top as address (processor sized)\n");
    printif("\"  [i]  ( - s ) enclose text up to next \" in a count prefixed string\n");
    printif("#   ( n - n' ) convert a digit from n\n");
    printif("#>   ( n - a c ) finish number sequence and return address and count\n");
    printif("#s   ( n - 0 ) convert all digits in n\n");
    printif("'  [i]  ( - a ) get execution address of following word\n");
    printif("(  [i]  start of comment till end of line or )\n");
    printif("*   ( n m - p ) multiply next data stack item by top and leave on top\n");
    printif("+   ( n m - p ) add top two data stack items and leave on top\n");
    printif("+b   ( b a - ) turn on b bits at address a: 0b10001 em +b\n");
    printif(",   ( n - ) allocate 1 cell and put n into it\n");
    printif("-   ( n m - p ) subtract top data stack item from next item and leave on top\n");
    printif("-b   ( b a - ) turn off b bits at address a: 0b10001 em -b\n");
    printif(".   ( n - ) print n in current number base\n");
    printif(".\"  [i]  print text up until next \"\n");
    printif(".b   ( n - ) print n in binary\n");
    printif(".d   ( n - ) print n in decimal\n");
    printif(".h   ( n - ) print n in hex\n");
    printif(".r   ( m n - ) print m in right field of n digits\n");
    printif(".s   print number of items on data stack\n");
    printif("/   ( n m - q ) divide next data stack item by top and leave on top\n");
    printif("/mod   ( n m - q r ) return divide and modulus from top item into next item\n");
    printif("0stats   reset machine stats\n");
    printif(":   start a macro definition\n");
    printif(";  [i]  end a macro\n");
    printif("<   ( n m - f ) leave a boolean on stack indicating if next is less than top\n");
    printif("<#   inititiate a number sequence\n");
    printif("=   ( n m - f ) leave a boolean on stack after equating top two data stack items\n");
    printif(">   ( n m - f ) leave a boolean on stack indicating if next is greater than top\n");
    printif(">r   ( n - ) (R - n ) push the top item of the data stack onto the return stack\n");
    printif("?dup   ( n - n n | - 0 ) duplicate top data stack item if not 0\n");
    printif("@   ( a - n ) return contents of memory using top stack item as the address (processor sized)\n");
    printif("[  [i]  exit macro mode\n");
    printif("]   enter macro mode\n");
    printif("abs   ( n - n|-n) top data stack item is made positive\n");
    printif("again  [i]  end of a continuous loop construct\n");
    printif("allot   ( n - ) reserve n bytes after end of dictionary\n");
    printif("and   ( n m - p ) bitwise AND top two data stack items and leave on top\n");
    printif("base   ( - a ) return address of number radix\n");
    printif("begin  [i]  start of a loop construct\n");
    printif("c!   ( c a - ) store next into memory using top as address (8 bit)\n");
    printif("c,   ( c - ) allocate and 1 byte and put value in it\n");
    printif("c@   ( a - c ) return contents of memory using top stack item as the address (8 bit)\n");
    printif("car   ( a - )copy and run an application from flash\n");
    printif("checksum   ( a n - c ) return checksum from a for n bytes\n");
    printif("cmove   ( s d n - ) move n bytes from s to d\n");
    printif("constant   ( n - ) give n a name\n");
    printif("copyapp   ( a - f ) copy application\n");
    printif("count   ( a - a' c ) leave first character and incremented address on stack\n");
    printif("cr   send end of line to output device\n");
    printif("date   show date and time\n");
    printif("dbg  [v]  set debug level 0 - quiet, 1 state changes, 2 traffic: 2 dbg c!\n");
    printif("decimal   interpret all subsequent numbers as decimal\n");
    printif("dh   ( a -- ) dump header parameters of an image\n");
    printif("drop   ( n - ) throw away the top data stack item\n");
    printif("dump   ( a n - ) dump n 16-byte rows of memory starting at address a\n");
    printif("dup   ( n - n n ) make a copy of the top data stack item\n");
    printif("else  [i]  otherwise part of an if statement\n");
    printif("emit   ( c - ) send c to output device\n");
    printif("end   display elapsed time since start\n");
    printif("endif  [i]  end of else or if statement\n");
    printif("erase   ( s n - ) erase n bytes from s\n");
    printif("execute   ( a - ) use the top data stack item as a function call\n");
    printif("exit  [i]  exit macro\n");
    printif("fill   ( s n x - )fill n bytes from s with x\n");
    printif("for  [i]  ( n - ) start of a loop which runs n times\n");
    printif("help   print list of one liners about words in dictionary\n");
    printif("here   ( - a ) return address of end of dictionary\n");
    printif("hex   interpret all following numbers as hex\n");
    printif("hold   ( c - ) hold a character in number sequence\n");
    printif("if  [i]  ( n - ) execute following code if top of stack is non-zero\n");
    printif("iflash  [v]  start of internal flash\n");
    printif("initsfp   initialize sfp firmware\n");
    printif("initsfpstats   zero out all sfp stats\n");
    printif("koff   turn off automatic key echo\n");
    printif("kon   turn on automatic key echo\n");
    printif("l!   (n a - )store next into memory using top as address (processor sized)\n");
    printif("l@   ( a - n )return contents of memory using top stack item as the address (32 bit)\n");
    printif("lf   dump out pool of frames\n");
    printif("literal  [i]  ( n - ) enter a literal value into a macro\n");
    printif("lm   list machines\n");
    printif("max   ( n m - n|m) leave maximum of top two stack items\n");
    printif("min    ( n m - n|m) leave minimum of top two stack items\n");
    printif("mod   ( n m - r ) modulus next data stack item by top and leave on top\n");
    printif("negate   ( n - -n ) two's complement of top data stack item\n");
    printif("next  [i]  end of a for loop\n");
    printif("nodestats   node stats\n");
    printif("not   ( n - n' ) invert all bits on the top data stack item\n");
    printif("or    ( n m - p ) bitwise OR top two data stack items and leave on top\n");
    printif("over   ( n m - n m n ) copy 2nd data stack item to top of data stack\n");
    printif("r   ( - n ) (R n - n ) copy the top item of the return stack onto the data stack\n");
    printif("r>    ( - n ) (R n - ) move top item on return stack to data stack\n");
    printif("reboot   reboot the software\n");
    printif("repeat  [i]  go back to the begin part\n");
    printif("reveal  [i]  allow macro to call itself\n");
    printif("runapp   ( a - f ) run application\n");
    printif("s!   ( h a - ) store next into memory using top as address (16 bit)\n");
    printif("s@   ( a - h ) return contents of memory using top stack item as the address (16 bit)\n");
    printif("setdate   ( Y M D - ) set the date\n");
    printif("settime   ( H M S - ) set the time\n");
    printif("sfpstats   show sfp statistics\n");
    printif("shift   ( n m - p ) shift n by m bit left for minus and right for positive\n");
    printif("sign   ( n - ) prepend sign to number sequence if n is negative\n");
    printif("sp!   ( ... - ) empty the data stack\n");
    printif("start   note the starting time\n");
    printif("stats   show time statistics on machine loop\n");
    printif("stmboot   invoke system bootloader so new firmware can be downloaded\n");
    printif("swap   ( n m - m n ) swap top two items on the data stack\n");
    printif("time   display local time\n");
    printif("type   ( a n - ) output n characters starting at a\n");
    printif("u<   ( u v - f ) leave a boolean on stack indicating if unsigned next is less than top\n");
    printif("u>   ( u b - f ) leave a boolean on stack indicating if unsigned next is greater than top\n");
    printif("until  [i]  ( n - ) go back to the begin statement if stack is zero\n");
    printif("variable   ( n - ) give n a place to be stored at a name\n");
    printif("version   display application name and build date/time\n");
    printif("vh   ( a -- ) check header checksum\n");
    printif("vi   ( a -- ) check image checksum\n");
    printif("while  [i]  ( n - ) conditional choice in a loop construct\n");
    printif("word   ( c - ) parse characters up to c from input to here\n");
    printif("words   list all words in dictionary\n");
    printif("xor   ( n m - p ) bitwise XOR top two data stack items and leave on top\n");
}
