// Command Line Interpreter  Robert Chapman III  Dec 6, 2016
// or call it TimbreTalk?

#include "machines.h"
#include "byteq.h"
#include "cli.h"

#include <string.h>

// structures
static QUEUE(DCELLS, dataStack);
static QUEUE(RCELLS, returnStack);
static BQUEUE(PAD_SIZE, padq); // safe place to format numbers

BQUEUE(EMITQ_SIZE, emitq);
BQUEUE(KEYQ_SIZE, keyq);

static Byte hereSpace[HERE_SPACE];
static Byte * hp = (Byte *)NULL, * hpStart = hereSpace, * hpEnd = &hereSpace[HERE_SPACE-1];
static Cell outp = 0;
static bool echo = true;
static Byte base = 10; // command line number radix
static Byte prompt[10]={PROMPTSTRING};
static Byte compiling = 0;
static tcbody * tick;
static tcode * ip;
static Byte interpretError = 0;
static header * wordlist = (header *)NULL; // list of words created from CLI

static struct { // text input buffer for parsing
	Cell in; // index into buffer
	Byte buffer[LINE_LENGTH + 1]; // extra room for zero at end
} tib;

Headless(lii); // for inline interpreters
Headless(branch);
Headless(zeroBranch);
Headless(minusBranch);
Headless(tor);

// data stack
Cell ret(void)  /* m - */
{
	return popq(dataStack);
}

void lit(Cell n)  /* - n */
{
	pushq(n, dataStack);
}

Cell depth()
{
	return queryq(dataStack);
}

void spStore(void) /* ... - */
{
	zeroq(dataStack);
}

void swap(void)  /* m n - n m */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);
	
	pushq(top, dataStack);
	pushq(next, dataStack);
}

void drop(void) /* n - */
{
	popq(dataStack);
}
void dup(void)  /* m - m m */
{
	pushq(p(dataStack), dataStack);
}

void over(void) /* m n - m n m */
{
	Cell top = popq(dataStack);
	Cell next = p(dataStack);
	
	pushq(top, dataStack);
	pushq(next, dataStack);
}

void questionDup(void)  /* n - [n] n */
{
	if (p(dataStack) != 0)
		dup();
}

// return stack
void rpStore(void)
{
	zeroq(returnStack);
}

void tor(void)  /* m - */
{
	pushq(popq(dataStack), returnStack);
}

void rat(void)  /* - m */
{
	pushq(p(returnStack), dataStack);
}

void rfrom(void)  /* - m */
{
	pushq(popq(returnStack), dataStack);
}

// logic
#define binary(op) 	\
	Cell top = popq(dataStack); \
	Cell next = popq(dataStack); \
	pushq(next op top, dataStack)
#define binaryInts(op) 	\
	Cell top = popq(dataStack); \
	Cell next = popq(dataStack); \
	pushq((Cell)((Integer)next op (Integer)top), dataStack)
#define unary(op) pushq( op popq(dataStack), dataStack)

void andOp(void)  /* m n - p */
{
	binary(&);
}

void orOp(void)  /* m n - p */
{
	binary(|);
}

void xorOp(void)  /* m n - p */
{
	binary(^);
}

void notOp(void)  /* m - n */
{
	unary(~);
}

void shiftOp(void) /* n m - p */
{
	Cell top = popq(dataStack); \
	Cell next = popq(dataStack); \

	if ((signed)top < 0)
		next >>= -(signed)top;
	else
		next <<= top;
	pushq(next, dataStack);
}

// math
void plusOp(void)  /* m \ n -- p */
{
	binary(+);
}

void minusOp(void)  /* m \ n -- p */
{
	binary(-);
}

void negateOp(void)  /* m -- n */
{
	unary(-);
}

void slashModOp(void)  /* n \ m -- remainder \ quotient */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	pushq(next % top, dataStack);
	pushq(next / top, dataStack);
}

void slashOp(void)  /* n \ m -- quotient */
{
	binary(/);
}

void modOp(void)  /* n \ m -- remainder */
{
	binary(%);
}

void starOp(void)  /* n \ m -- p */
{
	binary(*);
}

void absOp(void)  /* n -- n */
{
	if((Integer)p(dataStack) < 0)
		pushq((Cell)(-(Integer)popq(dataStack)), dataStack);
}

void maxOp(void)  /* n \ m -- p */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	if ((Integer)top > (Integer)next)
		pushq(top, dataStack);
	else
		pushq(next, dataStack);
}

void minOp(void)  /* n \ m -- p */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	if ((Integer)top < (Integer)next)
		pushq(top, dataStack);
	else
		pushq(next, dataStack);
}

// compare
void equals(void)  /* n \ m -- flag */
{
	binary(==);
}

void lessThan(void)  /* n \ m -- flag */
{
	binaryInts(<);
}

void greaterThan(void)  /* n \ m -- flag */
{
	binaryInts(>);
}

// memory
void error(void);

void hereSay(Byte * space, Cell size)
{
	hp = hpStart = space;
	hpEnd = space + size;
}

void here(void)  /* -- addr */
{
	lit((Cell)hp);
}

void allot(Cell n)
{
	if (hp + n + CUSHION < hpEnd)
		hp += n;
	else
		error();
}

void cliAllot(void)
{
	allot(ret());
}
void cComma(void)  /* n -- */
{
	*hp = (Byte)popq(dataStack);
	allot(1);
}

Cell align(Cell p)  /* a -- a' */
{
	struct{void*x; char y; void*z;}a;
	Cell z = (Cell)&a.z - (Cell)&a.y - sizeof(a.y); /* 1 or 3 */
	
	return (p + z) & ~z;
}

void aligned(void)
{
	hp = (Byte *)align((Cell)hp);
}

void comma(void)  /* n -- */
{
	Cell top = popq(dataStack);
	Cell * p = (Cell *)hp;
	aligned();
	*p = top;
	allot(sizeof(Cell));
}

void fetch(void)  /* a -- n */
{
	pushq(*(Cell *)popq(dataStack), dataStack);
}

void store(void)  /* n \ a -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);
	
	*(Cell *)top = next;
}

void longFetch(void) // a - lo \ hi
{
	Long l = *(Long *)popq(dataStack);
	
	pushq((Cell)(l & 0xFFFF), dataStack);
	pushq((Cell)(l>>16), dataStack);
}

void longStore(void) // lo \ hi \ a -
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);
	Cell third = popq(dataStack);

	*(Long *)top = ((Long)next<<16) + third;
}
	
void shortFetch(void) // a - n
{
	Short s = *(Short *)popq(dataStack);
	
	pushq((Cell)s, dataStack);
}

void shortStore(void) // n \ a -
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	*(Short *)top = (Short)next;
}

void byteFetch(void)  /* a -- c */
{
	Byte c = *(Byte *)popq(dataStack);
	
	pushq((Cell)c, dataStack);
}

void byteStore(void)  /* c \ a -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	*(Byte *)top = (Byte)next;
}

void plusBits(void)  /* bits \ addr -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	*(Byte *)top |= next;
}

void minusBits(void)  /* bits \ addr -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	*(Byte *)top = (Byte)(*(Byte *)top & ~next);
}

void byteMove(void)  /* src \ dest \ count -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);
	Cell third = popq(dataStack);

	memcpy((void *)next, (void *)third, top);
}

void byteFill(void)  /* addr \ count \ char -- */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);
	Cell third = popq(dataStack);

	memset((void *)third, (Byte)top, next);
}

void byteErase(void)  /* addr \ count -- */
{
	lit(0);
	byteFill();
}

// output stream
void emptyEmitq(void)
{
	zerobq(emitq);
}

void safeEmit(Byte c)
{
	static bool alreadyHere = false; // prevent overwrites

	if (fullbq(emitq)) {
		if (alreadyHere) // support blocking on first writer but dump after that
			return;
		alreadyHere = true;
		while (fullbq(emitq))
			OUTPUT_BLOCKED;  // sit here until sent
		alreadyHere = false;
	}
	pushbq(c, emitq);
}

void emitByte(Byte c)
{
	if ((c == BSPACE) || (c == DELETE)) {
		if (outp != 0)
			outp -= 1, c = BSPACE;
		else
			c = BEEP;
	}
	else if (c == CRETURN)  /* cursor return? */
		outp = 0;
	else if ((c != LFEED) && (c != BEEP))
		outp += 1;         /* not line feed, pacing character or bell? */
	safeEmit(c);
}

void emitOp(void)  /* char -- */
{
	emitByte((Byte)popq(dataStack));
}

void cursorReturn(void)
{
	emitByte(10);
	emitByte(13);
}

void maybeCr(void)
{
	if(outp != 0)
		cursorReturn();
}

void count(void)  /* addr -- addr' \ count */
{
	Byte * a = (Byte *)popq(dataStack);
	
	pushq((Cell)&a[1], dataStack);
	pushq((Cell)*a, dataStack);
}

void type(void)  /* addr \ count -- */
{
	Byte n = (Byte)popq(dataStack);
	Byte * a = (Byte *)popq(dataStack);
	
	while (n--)
		emitByte(*a++);
}

void spaces(int n)
{
	while (n-- > 0)
		emitByte(SPACE);
}

void bin(void)
{
	base = 2;
}

void oct(void)
{
	base = 8;
}

void decimal(void)
{
	base = 10;
}

void hex(void)
{
	base = 16;
}

void hold(void)  /* char -- */
{
	pushbq((Byte)popq(dataStack), padq);
}

void startNumberConversion(void)
{
	zerobq(padq);
}

void convertDigit(void)  /* n -- n */
{
	Cell n = popq(dataStack);
	Byte c = (Byte)(n % base);

	if (c > 9)
		c += 7;
	c += ZERO;
	pushbq(c, padq);

	n /= base;
	pushq(n, dataStack);
}

void convertNumber(void)  /* n -- n */
{
    do {
        convertDigit();
    } while(p(dataStack) != 0);
}

void sign(void)  /* m \ n -- n */
{
	Cell top = popq(dataStack);
	Cell next = popq(dataStack);

	if((Integer)next < 0)
		pushbq('-', padq);
	pushq(top, dataStack);
}

void endNumberConversion(void)  /* n -- addr \ count */
{
	Cell n = qbq(padq);
	Byte * a = &hp[LINE_LENGTH];
	
	popq(dataStack);
	pushq((Cell)a, dataStack);
	pushq(n, dataStack);

	while (qbq(padq))
		*a++ = popbq(padq);
	*a = 0; // add null terminator for use as a C string
}

void dotr(void)  /* n \ m -- */
{
	tor();

	startNumberConversion();
	if (base == 10) {
		dup();
		absOp();
		convertNumber();
		sign();
	} else
		convertNumber();
	endNumberConversion();

	rfrom();
	over();
	minusOp();
	lit(0);
	maxOp();
	spaces(popq(dataStack));
	type();
}

void dot(void)  /* n -- */
{
	lit(0);
	dotr();
	spaces(1);
}

void dotb(void)  /* n -- */ // print bits in number
{
	Cell n = ret();
	Cell mask = ~(((Cell)-1)>>1);

	spaces(1);
	for (Byte i = (sizeof(Cell) - 1)*8; i > 0; i -= 8) {
		Cell t = 0xff; // assignment makes 0xff long long otherwise shift fails!

		if (((t << i) & n) == 0)
			mask >>= 8;
		else // skip upper bytes if all zero
			break;
	}

	while (mask) {
		for (Byte i=8; i; i--) {
			if (n & mask)
				emitByte('1');
			else
				emitByte('0');
			mask >>= 1;
		}
		spaces(1);
	}
}

void dotd(void)  /* n -- */
{
	Byte b = base;

	decimal();
	dot();
	base = b;
}

void doth(void)  /* n -- */
{
	Byte b = base;

	hex();
	lit(sizeof(Cell)*2);
	dotr();
	spaces(1);
	base = b;
}

void msg(const char * m) // message in program space
{
	while (*m)
		emitByte(*m++);
}

void dots(void)  /* -- */
{
	Integer n = depth();

	if (n > DCELLS/2) // consider this as underflow
		n -= DCELLS + 1;
	lit(n);
	dot();
	msg(" stack items: ");
	if (n > 10)
		n = 10;

	for (Byte i=0; i < n; i++)
		tor();

	while (n-- > 0) {
		rfrom();
		dup();
		dot();
	}
}

Byte getBase(void)
{
	return base;
}

void setBase(Byte b)
{
	if (b <= 36) // highest base supported
		base = b;
}

// prompt
void setPrompt(const char *string)
{
	Byte length = (Byte)strlen(string);

	if (length > sizeof(prompt) + 1)
		length = sizeof(prompt) - 1;
	prompt[0] = length;
	memcpy(&prompt[1], string, length);
}

void dotPrompt(void)
{
	maybeCr();
	compiling ?	lit((Cell)"\002] ") : lit((Cell)prompt);
	count();
	type();
}

// compiler
void righBracket(void)
{
	compiling = 0x80;
}

void leftBracket(void)
{
	compiling = 0;
}

void compileIt(tcbody * t)
{
	lit((Cell)t);
	comma();
}

void executeIt(tcbody * t)
{
	tick = t;
	t->ii();
}

void execute(void) /* a - */
{
	executeIt((tcbody *)ret());
}

// inner interpreters
void vii()	/* -- ii */ // adddress returner
{
	lit((Cell)(tick->list));
}

void cii()	/* -- ii */ // constant retreiver
{
	lit(tick->list[0].lit);
}

void lii(void)  /* -- n */ // inline literals
{
	lit(ip++->lit);
}

void colonii() // macro threader
{
	tcode * save = ip;

	ip = tick->list;
	while((tick = ip++->call) != 0)
		tick->ii();
	ip = save;
}

void branch(void)
{
	ip = ip->branch;
}

void zeroBranch(void)  /* f -- */
{
	if (ret() == 0)
		branch();
	else
		ip++;
}

void minusBranch(void)
{
	Cell i = pullq(returnStack);

	if(i) {
		pushq(--i, returnStack);
		branch();
	}
	else
		ip++;
}

// parsing
void zeroTib(void)
{
	tib.in = 0;
}

void skip(Byte c) // skip c in input
{
	while (tib.buffer[tib.in] != 0 && tib.buffer[tib.in] == c)
		tib.in++;
}

void parse(Byte c) // parse string till char or 0 from input to here count prefixed
{
	Byte * input = &tib.buffer[tib.in];
	Byte * output = hp;

	while (*input != 0) {
		Byte b = *input++;

		if (b == c)
			break;
		*++output = b;
	}
	hp[0] = output - hp;	// count prefixed
	output[1] = 0;			// null terminate for usage as C string
	tib.in = input - tib.buffer;
}

Byte * parseWord(Byte c) // return C-string from input
{
	skip(c);
	parse(c);
	return &hp[1];
}

void comment(void)  /* char -- */ // scan input for end comment or 0
{
	Byte * input = &tib.buffer[tib.in];

	while (*input)
		if (*input++ == ')')
			break;
	tib.in = input - tib.buffer;
}

// dictionary words
/* CLI built words use this header structure:
 * [ link | name | II | list of other ticks ]

 * Prebuilt headers use:
 * [ name1 | name2 | ... | nameN ]
 * [ func1 | func2 | ... | funcN ]
 *
 * Address of II or funci is called the tick. ticks are executed or compiled.
 */
// search CLI list
header * searchWordlist(Byte * cstring)
{
	header * list = wordlist;

	while (list) {
		Byte * name = list->name;
		Byte length = strlen((char *)cstring);

		if ((name[0] & ~IMMEDIATE_BITS) == length) // smudged bit prevents matching bad headers
			if (0 == memcmp(&name[1], cstring, length))
				break;
		list = list->list;
	}
	return list;
}

// search prebuilt word lists; use hashed dictionaries to improve speed for large wordsets
Short searchNames(Byte * cstring, PGM_P dictionary) // return name number or 0 if not found
{
	Short index = 1;

	while(pgm_read_byte(dictionary)) {
		if (strcmp_P(cstring, dictionary) == 0)
			return index;
		index++;
		dictionary += strlen_P(dictionary) + 1;
	}
	return 0;
}

Byte searchDictionaries(Byte * cstring, tcbody ** t) // look through dictionaries arrays for word
{ // s -- a \ f
	Short index;

	index = searchNames(cstring, wordnames);
	if (index != 0) {
		*t = (tcbody *)&wordbodies[index-1];
		return NAME_BITS;
	}

	index = searchNames(cstring, constantnames);
	if (index != 0) {
		*t = (tcbody *)&constantbodies[2*(index-1)]; // array of two pointers
		return NAME_BITS;
	}

	index = searchNames(cstring, immediatenames);
	if (index != 0) {
		*t = (tcbody *)&immediatebodies[index-1];
		return IMMEDIATE_BITS;
	}

	return 0;
}

tcbody * link2tick(header * link)
{
	Byte length = link->name[0] & ~HEADER_BITS;
	Cell t = align((Cell)&link->name[1 + length]);

	return (tcbody *)t;
}

Byte lookup(Byte * cstring, tcbody ** t)
{
	header * result;

	result = searchWordlist(cstring);
	if (result != 0) {
		*t = link2tick(result);
		return result->name[0] & HEADER_BITS;
	}

	return searchDictionaries(cstring, t);
}

// Error recovery
void error(void)
{
	here();
	count();
	type();
	msg("<- eh?");
	interpretError = 1;
}

// Number conversion
Byte checkBase(Byte * cstring) // check for prefixes: 0X, 0x, 0C, 0c, 0B or 0b
{
	if (cstring[0] != 0 && cstring[1] != 0 && cstring[2] != 0) // count is longer than 2
		if (*cstring == '0') {  // and first digit is 0
			switch(cstring[1]) {
			case 'b': case 'B':	bin(); break;
			case 'c': case 'C':	oct(); break;
			case 'x': case 'X':	hex(); break;
			default: return 0;
			}
			return 2; // skip leading base change
		}
	return 0; // skip nothing
}

bool toDigit(Byte *n) // convert character to number according to base
{// covers all alphanumerics and bases
	Byte c = *n - '0';

	if (c > 9) {
		c -= 7;
		if (c > 35)
			c -= SPACE;
		if (c < 10)
			return false;
	}
	if (c >= base)
		return false;
	*n = c;
	return true;
}

Cell signDigits(Byte * cstring, bool sign) // convert string to number according to base
{
	Cell n = 0;

	if (*cstring == 0) {
		error();
		return 0;
	}

	while (*cstring) {
		Byte c = *cstring++;

		if (!toDigit(&c)) {
#ifdef FLOAT_SUPPORT
			if (c == '.') { // decimal point encountered - try for mantissa
				float f = 0;

				while (*cstring) // start at end of string to work back to decimal
					cstring++;

				while (*--cstring != '.') {
					c = *cstring;
					if (!toDigit(&c)) {
						error();
						return n;
					}
					f = (f+c)/base;
				}
				f = f + n;
				if (sign)
					f = -f;
				*(float *)&n = f;
				return n;
			}
#endif
			error();
			return n;
		}
		n = n*base + c;
	}
	if (sign)
		n = -n;
	return n;
}

Cell stringNumber(Byte * cstring)
{
	Cell n;
	Byte b = base;
	bool sign = (*cstring == '-');

	if (sign)
		cstring++;
	cstring += checkBase(cstring);
	n = signDigits(cstring, sign);
	base = b;
	return n;
}

// threaded interpreters
void literal(Cell n)
{
	lit(n);
	if (compiling) {
		compileIt(&_lii);
		comma();
	}
}

// interpreter
void quit(void)
{
	interpretError = 0;
	spStore();
	rpStore();
	zeroTib();
	leftBracket();
	cursorReturn();
}

void interpret(void)
{
	while (tib.buffer[tib.in] != 0) {
		tcbody * t;
		Byte headbits;
		Byte * cstring;

		cstring = parseWord(SPACE);
		headbits = lookup(cstring, &t);
		if (headbits != 0)
			headbits == compiling ? compileIt(t) : executeIt(t);
		else {
			Cell n = stringNumber(cstring);

			if (interpretError) {
				quit();
				break;
			}
			literal(n);
		}
	}
}

// control flow
void compileAhead(void)
{
	compileIt(&_branch);
	here();
	lit(0);
	comma();
}

void compileIf(void)
{
	compileIt(&_zeroBranch);
	here();
	lit(0);
	comma();
}

void compileEndif(void)
{
	here();
	swap();
	store();
}

void compileElse(void)
{
	compileAhead();
	swap();
	compileEndif();
}

void compileBegin(void)
{
	here();
}

void compileAgain(void)
{
	compileIt(&_branch);
	comma();
}

void compileWhile(void)
{
	compileIf();
}

void compileRepeat(void)
{
	swap();
	compileAgain();
	compileEndif();
}

void compileUntil(void)
{
	compileIt(&_zeroBranch);
	comma();
}

void compileFor(void)
{
	compileIt(&_tor);
	compileAhead();
	here();
}

void compileNext(void)
{
	swap();
	compileEndif();
	compileIt(&_minusBranch);
	comma();
}

void compileExit(void)
{
	lit(0);
	comma();
}

// strings
void makeString(char c)
{
	parse(c);
	here();
	dup();
	byteFetch();
	lit(1);
	plusOp();
	allot(ret());
	aligned();
}

void quote(void)
{
	if (compiling)
		compileAhead();

	makeString('"');

	if (compiling) {
		swap();
		compileEndif();
		literal(ret());
	}
}

// input stream
void autoEchoOn(void)
{
    echo = true;
}

void autoEchoOff()
{
    echo = false;
}

void emptyKeyq(void)
{
	zerobq(keyq);
}

void cli(void)
{
	if (qbq(keyq) == 0)
		return;

	Byte key = pullbq(keyq);

	switch (key) {
	case LFEED: // ignore line feeds
		return;
	case BSPACE:
	case DELETE: // backspace or delete
		if (tib.in != 0)
			tib.in -= 1;
		else
			key = BEEP;
		break;
	case QUOTE:
		quote();
		break;
	case CRETURN:
	case 0:  // a cursor return
		key = 0;
		tib.buffer[tib.in] = key;
		outp = 0;
                if (echo)
                    spaces(1);
		zeroTib();
		interpret();
		zeroTib();
		tib.buffer[tib.in] = 0;
		dotPrompt();
		return;
	default:
		if ( key < ESCAPE )
			key = BEEP;
		else if ( tib.in < LINE_LENGTH ) { // check in not out!
			tib.buffer[tib.in] = key;
			tib.in++;
		}
		else
			key = BEEP;
	}
        if (echo)
            emitByte(key);
}

// defining words
// make header name from null terminated string no header bits, easier searching, count becomes strlen; link;string;->:ii;list ; or string is a pointer to a string and then the header is a fixed size structure;
static void makeHeader(void)
{
	aligned();
	here();
	lit((Cell)wordlist);
	comma();
	wordlist = (header *)ret();
	parseWord(SPACE);
	*hp |= NAME_BITS;
	allot((*hp & ~HEADER_BITS) + 1);
	aligned();
}

void colon(void)
{
	makeHeader();
	wordlist->name[0] |= SMUDGE_BITS;
	lit((Cell)colonii);
	comma();
	righBracket();
}

void semiColon(void)
{
	compileExit();
	wordlist->name[0] &= ~SMUDGE_BITS;
	leftBracket();
}

void constant(void)  /* n -- */
{
	makeHeader();
	lit((Cell)cii);
	comma();
	comma();
}

void create(void)
{
	makeHeader();
	lit((Cell)vii);
	comma();
}

void variable(void)  /* n -- */
{
	create();
	comma();
}

// Tools
static void list_dictionaries(void) // list words in dictionaries
{
	PGM_P dictionary, *dictionaries[] = {constantnames, wordnames, immediatenames};
	Byte i, c;

	for (i=0; i< 3; i++) {
		dictionary = dictionaries[i];
		while((c = pgm_read_byte(dictionary++)) != 0) {
			do {
				emitByte(c);
			} while ((c = pgm_read_byte(dictionary++)) != 0);
			spaces(2);
		}
	}
}

void words(void) // list user words and system dictionaries
{
	header * list = wordlist;

	cursorReturn();
	while (list) {
		Byte * name = list->name;
		Byte length = *name++ & ~IMMEDIATE_BITS;

		while(length--)
			emitByte(*name++);

		spaces(2);
		list = list->list;
	}
	list_dictionaries();
}

// reset all
void resetCli(void)
{
	hereSay(hpStart, hpEnd - hpStart);
	wordlist = NULL;
	decimal();
	quit();
	dotPrompt();
}

// TODO: group by function; factor out magic numbers; improve names; reduce coupling so CLI can ignore parts; static functions; float support
// forget last definition if it encroaches; for bigger test scripts. support test scripting.
// need to test at the functional level with full dictionary
// need to verify here space overflow protection; perhaps other overflow protections too
