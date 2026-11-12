// Command Line Interpreter  Robert Chapman III  Dec 6, 2016
// or call it TimbreTalk?

#include "cli.h"
#include "byteq.h"
#include "tea.h"
#include "printers.h"
#include "ttypes.h"
#include "project_defs.h"

#include <string.h>
#include <ctype.h>
#include <setjmp.h>

Event EmitEvent = {no_action};
Event KeyEvent = {run_cli};

// structures; could use one end of a queue for data and the other end for return
static QUEUE(DCELLS, dataStack);
static QUEUE(RCELLS, returnStack);
static BQUEUE(PAD_SIZE, padq); // safe place to format numbers

// A data stack breach (push() finds it full, pop() finds it empty) means
// whatever word is mid-execution is working from state that isn't what
// it expects - its remaining C code has no way to know the breach
// happened and will keep going, using default/garbage values as if they
// were real. Concretely: pop()'s old underflow behavior returned 0 as a
// "safe" placeholder, but stringLength() and type() (among others) use
// their popped values as pointers with no validation - strlen((char*)0)
// or *((char*)garbage) both fault immediately. Confirmed live: a single
// line that pushes past DCELLS then immediately runs `dup dup strlen
// type` segfaults, with the exact crash site (stringLength() vs. type())
// varying run to run depending on heap layout - a clear sign this was
// undefined behavior, not a "sometimes it prints a message" edge case.
//
// The only way to stop the current word's C code from running any
// further - short of auditing every stack consumer in the dictionary for
// pointer validation, which the next new word could just as easily lack
// - is to unwind back out of it entirely. interpret_word() is the
// natural place to catch that: it is the one word-at-a-time boundary
// every path (interpret(), evaluate(), the normal interpreter loop) goes
// through before a word's own code runs. push()/pop() longjmp here after
// resetCli() has already put the machine back in a known-good state;
// interpret_word() itself just returns, and since resetCli() cleared the
// TIB, the interpreter loop that called it naturally stops trying to
// continue the line that caused the fault instead of needing to notice
// anything went wrong.
static jmp_buf stack_fault;

BQUEUE(EMITQ_SIZE, emitq);
BQUEUE(KEYQ_SIZE, keyq);

static Byte hereSpace[HERE_SPACE];
static Byte *hp = (Byte*)NULL, *hpStart = hereSpace, *hpEnd = &hereSpace[HERE_SPACE - 1];
static int outp = 0; // output characters since CR
static bool keyEcho = true; // echo keys typed in
static bool lineEcho = true; // used to turn off echo for a line; reset after every line
static Byte base = 10; // command line number radix
static Byte prompt[10] = { PROMPTSTRING };
static Byte compiling = 0;
static tcbody* tick;
static tcode* ip;
static Byte interpretError = 0, errors = 0;
static header* wordlist = (header*)NULL; // list of words created from CLI

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
//
// TOP (peek without popping) used to read straight from the queue via
// p(dataStack) with no depth check, unlike pop(). That's the same class
// of fault as the one push()/pop() now guard against, just reached a
// different way: `1 *` (one operand, `*` needs two) pops its one real
// operand via `Cell top = pop()` inside the binary() macro, leaving the
// stack empty, and then reads its *other* operand through TOP - which,
// unchecked, silently returned whatever stale value was still sitting in
// that queue slot instead of reporting underflow. Several TOP callers
// (@, s@, c@) go a step further and dereference that value as a pointer.
// Checking depth here, once, protects all of them - dup/over/sign-check/
// the binary and unary math ops/@ and friends - the same way pop() now
// protects every word that pops its operands explicitly.
static Cell checkedTop(void) {
    if (queryq(dataStack) == 0) {
        msg(" stack underflow - resetcli " );
        resetCli();
        longjmp(stack_fault, 1);
    }
    return p(dataStack);
}
#define TOP checkedTop()

void cliFault(const char * m) {
    msg(m);
    resetCli();
    longjmp(stack_fault, 1);
}

#ifdef FLOAT_DOUBLE
// Visit every cell on both stacks, leaving them as found: pulling the
// oldest cell and pushing it back as the newest, depth times over, brings
// each queue back to its starting order.
static void markQueue(Cell * q, void (*mark)(Cell)) {
    for (Cell n = queryq(q); n; n--) {
        Cell c = pullq(q);
        mark(c);
        pushq(c, q);
    }
}

void cliMarkStacks(void (*mark)(Cell)) {
    markQueue(dataStack, mark);
    markQueue(returnStack, mark);
}
#endif

static void push(Cell c) {
    if (fullbq(dataStack)) {
        msg(" stack overflow - resetcli " );
        resetCli();
        longjmp(stack_fault, 1);
    }
    pushq(c, dataStack);
}

static Cell pop() {
    if (queryq(dataStack))
        return popq(dataStack);
    msg(" stack underflow - resetcli " );
    resetCli();
    longjmp(stack_fault, 1);
}

Cell ret() /* m - */
{
    return pop();
}

void lit(Cell n) /* - n */
{
    push(n);
}

Cell depth()
{
    return queryq(dataStack);
}

void spStore() /* ... - */
{
    zeroq(dataStack);
}

void swap() /* m n - n m */
{
    Cell top = pop();
    Cell next = pop();

    push(top);
    push(next);
}

void drop() /* n - */
{
    pop();
}

void dup() /* m - m m */
{
    push(TOP);
}

void over() /* m n - m n m */
{
    Cell top = pop();
    Cell next = TOP;

    push(top);
    push(next);
}

void questionDup() /* n - [n] n */
{
    if (TOP != 0)
        dup();
}

// return stack
void rpStore()
{
    zeroq(returnStack);
}

void tor() /* m - */
{
    if (fullbq(returnStack)) {
        msg(" return stack overflow " );
        quit();
    } else
        pushq(pop(), returnStack);
}

void rat() /* - m */
{
    push(p(returnStack));
}

void rfrom() /* - m */
{
    if (queryq(returnStack))
        push(popq(returnStack));
    else {
        msg(" return stack underflow " );
        quit();
    }
}

// logic
#define binary(op)               \
    Cell top = pop();  \
    writep(TOP op top, dataStack)
#define binaryInts(op)           \
    Cell top = pop();  \
    writep((Cell)((Integer)TOP op(Integer) top), dataStack)
#define unary(op) writep(op TOP, dataStack)

void andOp() /* m n - p */
{
    binary(&);
}

void orOp() /* m n - p */
{
    binary(|);
}

void xorOp() /* m n - p */
{
    binary (^);
}

void notOp() /* m - n */
{
    unary(~);
}

void shiftOp() /* n m - p */
{
    Cell top = pop();
    Cell next = pop();

    if ((signed)top < 0)
        next >>= -(signed)top;
    else
        next <<= top;
    push(next);
}

// math
void plusOp() /* m \ n -- p */
{
    binary(+);
}

void minusOp() /* m \ n -- p */
{
    binary(-);
}

void negateOp() /* m -- n */
{
    unary(-);
}

void slashModOp() /* n \ m -- remainder \ quotient */
{
    Cell top = pop();
    Cell next = pop();

    if (top != 0) {
        push(next % top);
        push(next / top);
    } else {
        push(0);
        push(0);
        msg(" div/0 ");
    }

}

void slashOp() /* n \ m -- quotient */
{
    Cell top = pop();
    Cell next = pop();

    if (top != 0)
        top = next / top;
    else {
        top = 0;
        msg(" div/0 ");
    }
    push(top);
}

void modOp() /* n \ m -- remainder */
{
    Cell top = pop();
    Cell next = pop();

    if (top != 0)
        top = next % top;
    else {
        top = 0;
        msg(" mod/0 ");
    }

    push(top);
}

void starOp() /* n \ m -- p */
{
    binary(*);
}

void absOp() /* n -- n */
{
    if ((Integer)TOP < 0)
        push((Cell)(-(Integer)pop()));
}

void maxOp() /* n \ m -- p */
{
    Cell top = pop();
    Cell next = pop();

    if ((Integer)top > (Integer)next)
        push(top);
    else
        push(next);
}

void minOp() /* n \ m -- p */
{
    Cell top = pop();
    Cell next = TOP;

    if ((Integer)top < (Integer)next)
        writep(top, dataStack);
    else
        writep(next, dataStack);
}

// compare
void equals() /* n \ m -- flag */
{
    binary(==);
}

void lessThan() /* n \ m -- flag */
{
    binaryInts(<);
}

void greaterThan() /* n \ m -- flag */
{
    binaryInts(>);
}

// memory
void error_occurred();

void hereSay(Byte* space, Cell size)
{
    hp = hpStart = space;
    hpEnd = space + size;
}

Long here_left() { return hpEnd - hp; }

void here() /* -- addr */
{
    lit((Cell)hp);
}

// n is signed in practice: `here - allot` (the harness's marker, and the
// usual way to forget a string) passes a negative count to rewind. The old
// check was `hp + n + CUSHION < hpEnd` with n unsigned, which only guarded
// the top - a negative-looking n wrapped, passed, and moved hp below
// hpStart (or, for a large one, far outside here-space entirely), after
// which everything that writes at hp (c, , quoted strings, the number
// buffer in endNumberConversion) lands in whatever memory sits there.
// Confirmed live on the host build: `-100000 allot` then `here .h`
// segfaults. Both ends are checked now; hp stays in [hpStart, hpEnd-CUSHION).
void allot(Cell n)
{
    intptr_t d = (intptr_t)n;
    bool fits = d < 0 ? (Cell)(-d) <= (Cell)(hp - hpStart)
                      : (Cell)d + CUSHION < (Cell)(hpEnd - hp);
    if (fits)
        hp += d;
    else
        error_occurred();
}

void cliAllot()
{
    allot(ret());
}

void cComma() /* n -- */
{
    *hp = (Byte)pop();
    allot(1);
}

#define ALIGNMENT 4

Cell align(Cell p) { /* a -- a' */
    return (p + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

void aligned()
{
    hp = (Byte*)align((Cell)hp);
}

void comma() /* n -- */
{
    Cell top = pop();
    Cell* p = (Cell*)hp;
    aligned();
    *p = top;
#ifdef FLOAT_DOUBLE
    floatPin(top); // literals, constants and variables hold floats for good
#endif
    allot(sizeof(Cell));
}

void fetch() /* a -- n */
{
    writep(*(Cell*)TOP, dataStack);
}

void store() /* n \ a -- */
{
    Cell top = pop();
    Cell next = pop();

#ifdef FLOAT_DOUBLE
    // A float stored into a variable must outlive the stacks. Only done
    // inside dictionary space, where variables live: reading the old value
    // of a hardware register could have side effects.
    if ((Byte*)top >= hereSpace && (Byte*)top <= hpEnd) {
        floatPin(next); // before unpinning, in case it is the same float
        floatUnpin(*(Cell*)top);
    }
#endif
    *(Cell*)top = next;
}

void shortFetch() // a - n
{
    writep((Cell)*(Short*)TOP, dataStack);
}

void shortStore() // n \ a -
{
    Cell top = pop();
    Cell next = pop();

    *(Short*)top = (Short)next;
}

void byteFetch() /* a -- c */
{
    writep((Cell)*(Byte*)TOP, dataStack);
}

void byteStore() /* c \ a -- */
{
    Cell top = pop();
    Cell next = pop();

    *(Byte*)top = (Byte)next;
}

void plusBits() /* bits \ addr -- */
{
    Cell top = pop();
    Cell next = pop();

    *(Byte*)top |= next;
}

void minusBits() /* bits \ addr -- */
{
    Cell top = pop();
    Cell next = pop();

    *(Byte*)top = (Byte)(*(Byte*)top & ~next);
}

void byteMove() /* src \ dest \ count -- */
{
    Cell top = pop();
    Cell next = pop();
    Cell third = pop();

    memcpy((void*)next, (void*)third, top);
}

void byteFill() /* addr \ count \ char -- */
{
    Cell top = pop();
    Cell next = pop();
    Cell third = pop();

    memset((void*)third, (Byte)top, next);
}

void byteErase() /* addr \ count -- */
{
    lit(0);
    byteFill();
}

// output stream
void emptyEmitq()
{
    zerobq(emitq);
}

void safeEmit(Byte c)
{
    if (fullbq(emitq)) {
    	static bool alreadyHere = false; // prevent overwrites
        if (alreadyHere) // support blocking on first writer but dump after that
            return;
        alreadyHere = true;
        while (fullbq(emitq)) {
            OUTPUT_BLOCKED; // sit here until sent
            action_slice();
        }
        alreadyHere = false;
    }
    safe( pushbq(c, emitq); )
    (*EmitEvent)();
}

void emitByte(Byte c)
{
    if ((c == BSPACE) || (c == DELETE)) {
        if (outp != 0)
            outp -= 1, c = BSPACE;
        else
            c = BEEP;
    } else if (c == CRETURN || (c == LFEED)) /* cursor return? */
        outp = 0;
    else if ((c != LFEED) && (c != BEEP))
        outp += 1; /* not line feed, pacing character or bell? */
    safeEmit(c);
}

void emitOp() /* char -- */
{
    emitByte((Byte)pop());
}

void cursorReturn()
{
    emitByte('\n');
}

void maybeCr()
{
    if (outp != 0)
        cursorReturn();
}

int getCursor()
{
    return outp;
}

void msg(const char* m) // message in program space
{
    while (*m)
        emitByte(*m++);
}

void type() /* addr \ count -- */
{
    Byte n = (Byte)pop();
    Byte* a = (Byte*)pop();

    while (n--)
        emitByte(*a++);
}

void stringLength() /* a - c */
{
    lit(strlen((char*)ret()));
}

void dot_quote() { /* s - */
    lit(strlen((char*)TOP));
    type();
}

void spaces(int n)
{
    while (n-- > 0)
        emitByte(SPACE);
}

void bin()
{
    base = 2;
}

void oct()
{
    base = 8;
}

void decimal()
{
    base = 10;
}

void hex()
{
    base = 16;
}

void hold() /* char -- */
{
    pushbq((Byte)pop(), padq);
}

void startNumberConversion()
{
    zerobq(padq);
}

void convertDigit() /* n -- n */
{
    Cell n = pop();
    Byte c = (Byte)(n % base);

    if (c > 9)
        c += 7;
    c += ZERO;
    pushbq(c, padq);

    n /= base;
    push(n);
}

void convertNumber() /* n -- n */
{
    do {
        convertDigit();
    } while (TOP != 0);
}

void sign() /* m \ n -- n */
{
    Cell top = pop();
    Cell next = pop();

    if ((Integer)next < 0)
        pushbq('-', padq);
    push(top);
}

void endNumberConversion() /* n -- addr \ count */
{
    if (hp + LINE_LENGTH > hpEnd) { // prevent overflow of here space
        error_occurred();
        quit(); // reset everything
        return;
    }
    Byte* a = &hp[LINE_LENGTH];
    Cell n = qbq(padq);

    pop();
    push((Cell)a);
    push(n);

    while (qbq(padq))
        *a++ = popbq(padq);
    *a = 0; // add null terminator for use as a C string
}

// Upper bound on .r's padding count. field_width - digit_count is clamped
// to [0, DOTR_MAX_PAD] before reaching spaces(), which otherwise has no
// bound of its own - it just emits bytes one at a time until its count
// hits zero. A garbage or absurd field width (a sentinel probe value,
// a typo, anything of that shape) used to turn into a spaces() loop in
// the billions: not a crash, just the CPU faithfully emitting bytes for
// as long as it takes, which is indistinguishable from a hang to anyone
// watching the terminal. Confirmed live: this exact shape of call left a
// real board needing a physical reset. 80 is generously more than any
// real terminal field width this CLI ever prints.
#define DOTR_MAX_PAD 80

void dotr() /* n \ m -- */
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
    lit(DOTR_MAX_PAD);
    minOp();
    spaces(pop());
    type();
}

void dot() /* n -- */
{
    lit(0);
    dotr();
    spaces(1);
}

void dotb() /* n -- */ // print bits in number
{
    Cell n = ret();
    Cell mask = ~(((Cell)-1) >> 1);

    spaces(1);
    for (Byte i = (sizeof(Cell) - 1) * 8; i > 0; i -= 8) {
        Cell t = 0xff; // assignment makes 0xff long long otherwise shift fails!

        if (((t << i) & n) == 0)
            mask >>= 8;
        else // skip upper bytes if all zero
            break;
    }

    while (mask) {
        for (Byte i = 8; i; i--) {
            if (n & mask)
                emitByte('1');
            else
                emitByte('0');
            mask >>= 1;
        }
        spaces(1);
    }
}

void dotd() /* n -- */
{
    Byte b = base;

    decimal();
    dot();
    base = b;
}

void doth() /* n -- */
{
    Byte b = base;

    hex();
    lit(sizeof(Cell) * 2);
    dotr();
    spaces(1);
    base = b;
}

void dots() /* -- */
{
    Integer n = depth();

    if (n > DCELLS / 2) // consider this as underflow
        n -= DCELLS + 1;
    lit(n);
    dot();
    msg(" stack items: ");
    if (n > 10)
        n = 10;

    for (Byte i = 0; i < n; i++)
        tor();

    while (n-- > 0) {
        rfrom();
        dup();
#ifdef FLOAT_DOUBLE
        if (isFloat(TOP)) { // a tagged index says this cell is a float
            cliFdot();
            continue;
        }
#endif
        dot();
    }
}

void show_rstack() {
    msg(" return depth: ");
    lit(queryq(returnStack));
    dot();
}

Byte getBase()
{
    return base;
}

void setBase(Byte b)
{
    if (b <= 36) // highest base supported
        base = b;
}

// prompt
void setPrompt(const char* string)
{ // truncate if too long
    prompt[0] = 0;
    strncat((char*)prompt, string, sizeof(prompt) - 1);
}

void dotPrompt()
{
    maybeCr();
    compiling ? msg("] ") : msg((char*)prompt);
}

// compiler
void rightBracket()
{
    compiling = 0x80;
}

void leftBracket()
{
    compiling = 0;
}

void compileIt(tcbody* t)
{
    lit((Cell)t);
    comma();
}

void executeIt(tcbody* t)
{
    tick = t;
    t->ii();  // use actionRun to get stats
}

void execute() /* a - */
{
    executeIt((tcbody*)ret());
}

// inner interpreters
void vii() /* -- ii */ // adddress returner
{
    lit((Cell)(tick->list));
}

void cii() /* -- ii */ // constant retreiver
{
    lit(tick->list[0].lit);
}

void lii() /* -- n */ // inline literals
{
    lit(ip++->lit);
}

// should return stack be used for loops as well as return?
// consider a different queue and better integration
// with ip and tick.
// nest:  ipstack <- ++ip <- tick <- ip->call
// unnest: 
// with multiple ipstacks, multiple macros could be run
// this might be the way to integrate TEA and macros
// multiple actions at different times
// nap model is a hold back but perhaps simpler for user
// limited to single machines
// use TEA for multiple samples and reports
// Eliminate tick by incrementing ip before getting next item
//  instead of copying to tick for cii,vii, they can use ip
// stuffing to ipStack would be queueing actions.
void colonii() { // macro threader
    if (fullq(returnStack)) {
        msg(" return stack overflow " );
        quit();
    } else {
        pushq((Cell)ip, returnStack);
        ip = tick->list;
    }
}

bool executing() {
    // Words run from here a step at a time, after interpret_word() has
    // returned, so its setjmp is gone by then; a stack fault inside a :
    // word needs its own landing point or longjmp() jumps into a dead
    // stack frame. resetCli() has already cleared ip and both stacks.
    if (setjmp(stack_fault))
        return false;
    if (ip && ip->lit)
        executeIt(ip++->call);
    else if (queryq(returnStack))
        ip = (tcode*)popq(returnStack);
    else
        return false;
    return true;
}

void branch()
{
    ip = ip->branch;
}

void zeroBranch() /* f -- */
{
    if (ret() == 0)
        branch();
    else
        ip++;
}

void minusBranch()
{
    if (queryq(returnStack)) {
        Cell i = popq(returnStack);
        if (i) {
            pushq(--i, returnStack);
            branch();
        } else
            ip++;
    } else {
        msg( " return stack underflow " );
        quit();
    }
}

// parsing
void zeroTib()
{
    tib.in = 0;
    tib.buffer[tib.in] = 0;
}

void end_tib() {
    tib.buffer[tib.in] = 0;
    tib.in = 0;
}

Byte peek()
{
    return tib.buffer[tib.in];
}

void skip(Byte c) // skip c in input
{
    while (peek() != 0 && peek() == c)
        tib.in++;
}

void parse(Byte c) // parse string till char or 0 from input to here count prefixed
{
    Byte* input = &tib.buffer[tib.in];
    Byte* output = hp;

    while (*input != 0) {
        Byte b = *input++;

        if (b == c || output >= hpEnd - 1)
            break;
        *++output = b;
    }
    hp[0] = output - hp; // count prefixed
    output[1] = 0; // null terminate for usage as C string
    tib.in = input - tib.buffer;
}

Byte* parseWord(Byte c) // return C-string from input
{
    skip(c);
    parse(c);
    return &hp[1];
}

void comment() /* char -- */ // scan input for end comment or 0
{
    Byte* input = &tib.buffer[tib.in];

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
header* searchWordlist(Byte* cstring)
{
    header* list = wordlist;
    Byte length = strlen((char*)cstring);

    while (list) {
        Byte* name = list->name;

        if ((name[0] & ~IMMEDIATE_BITS) == length) // smudged bit prevents matching bad headers
            if (0 == strcasecmp((char *)&name[1], (char *)cstring))
                break;
        list = list->list;
    }
    return list;
}

// search prebuilt word lists; use hashed dictionaries to improve speed for large wordsets
Short searchNames(Byte* cstring, PGM_P dictionary) // return name number or 0 if not found
{
    Short index = 1;

    while (pgm_read_byte(dictionary)) {
        if (strcasecmp((char *)cstring, dictionary) == 0)
            return index;
        index++;
        dictionary += strlen_P(dictionary) + 1;
    }
    return 0;
}

Byte searchDictionaries(Byte* cstring, tcbody** t) // look through dictionaries arrays for word
{ // s -- a \ f
    Short index;

    index = searchNames(cstring, wordnames);
    if (index != 0) {
        *t = (tcbody*)&wordbodies[index - 1];
        return NAME_BITS;
    }

    index = searchNames(cstring, constantnames);
    if (index != 0) {
        *t = (tcbody*)&constantbodies[index - 1];
        return NAME_BITS;
    }

    index = searchNames(cstring, immediatenames);
    if (index != 0) {
        *t = (tcbody*)&immediatebodies[index - 1];
        return IMMEDIATE_BITS;
    }

    return 0;
}

tcbody* link2tick(header* link)
{
    Byte length = link->name[0] & ~HEADER_BITS;
    Cell t = align((Cell)&link->name[1 + length + 1]); // bits, terminator

    return (tcbody*)t;
}

Byte lookup(Byte* cstring, tcbody** t)
{
    header* result;

    result = searchWordlist(cstring);
    if (result != 0) {
        *t = link2tick(result);
        return result->name[0] & HEADER_BITS;
    }

    return searchDictionaries(cstring, t);
}

// Error recovery
void error_occurred()
{
    msg((char*)&hp[1]);
    msg("<- eh?");
    errors = interpretError = 1;
}

void zero_errors() { errors = 0; }
Byte get_errors() { return errors; }

// Number conversion
Byte checkBase(Byte* cstring) // check for prefixes: 0X, 0x, 0C, 0c, 0B or 0b
{
    if (cstring[0] == '0' && cstring[1] != 0 && cstring[2] != 0) {// count is longer than 2
		switch (tolower(cstring[1])) {
		case 'b':
			bin();
			break;
		case 'c':
			oct();
			break;
		case 'x':
			hex();
			break;
		default:
			return 0;
		}
		return 2; // skip leading base change
    }
    return 0; // skip nothing
}

bool toDigit(Byte* n) // convert character to number according to base
{ // covers all alphanumerics and bases
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

#ifdef FLOAT_SUPPORT
// Scales f by 10^exponent using repeated multiply/divide rather than
// powf(), so float support pulls in no new library dependency beyond
// what float.c already links. exponent is clamped to a small, bounded
// range first - not because larger exponents are invalid input, but
// because the *correct* result for anything past this range is exactly
// what clamping produces anyway (0 or +-inf, float32 cannot represent
// finite values outside roughly 1e-45..3.4e38), and a loop bounded by an
// unchecked parsed value is exactly the bug ".r"'s padding count turned
// out to be elsewhere in this file. 64 is comfortably past float32's own
// range in either direction.
#ifdef FLOAT_DOUBLE
#define MAX_DECIMAL_EXPONENT 330 // past float64's range, same reasoning
#else
#define MAX_DECIMAL_EXPONENT 64
#endif

static Real scaleByPow10(Real f, Integer exponent)
{
    if (exponent > MAX_DECIMAL_EXPONENT)
        exponent = MAX_DECIMAL_EXPONENT;
    else if (exponent < -MAX_DECIMAL_EXPONENT)
        exponent = -MAX_DECIMAL_EXPONENT;

    while (exponent > 0) {
        f *= 10;
        exponent--;
    }
    while (exponent < 0) {
        f /= 10;
        exponent++;
    }
    return f;
}
#endif

Cell signDigits(Byte* cstring, bool sign) // convert string to number according to base
{
    Cell n = 0;
#ifdef FLOAT_SUPPORT
    // The whole part of a float, kept apart from n since a 64 bit float
    // can hold whole parts that would overflow a 32 bit Cell.
    Real whole = 0;
#endif

    if (*cstring == 0) {
        error_occurred();
        return 0;
    }

#ifdef FLOAT_SUPPORT
    // Exponent notation ("1e10", "1.5e-3") is only meaningful in decimal -
    // 'e'/'E' is itself a valid hex digit, so this must not fire outside
    // base 10, or a legitimate hex literal like "0xE" would be misread.
    //
    // Located and stripped up front, before the digit loop below ever
    // runs, because the '.' branch further down scans backward from the
    // *end* of the string to find the decimal point, assuming everything
    // from there to the end is a fractional digit - an exponent suffix
    // sitting past the fraction would break that assumption. Truncating
    // the string here (restored before any error_occurred(), so a bad
    // literal is still reported exactly as typed) lets the rest of this
    // function parse the mantissa exactly as it did before exponents
    // existed, with the exponent applied to the result afterward.
    Byte* expAt = NULL;
    Byte expOriginal = 0;
    bool haveExponent = false;
    Integer exponent = 0;
    if (base == 10) {
        for (Byte* scan = cstring; *scan; scan++) {
            if (*scan == 'e' || *scan == 'E') {
                expAt = scan;
                break;
            }
        }
    }
    if (expAt) {
        Byte* d = expAt + 1;
        bool expSign = (*d == '-');
        if (expSign || *d == '+')
            d++;
        Byte* firstDigit = d;
        while (*d >= '0' && *d <= '9') {
            // Capped well short of overflowing Integer during accumulation
            // itself - scaleByPow10() clamps to +-MAX_DECIMAL_EXPONENT
            // regardless, so any value already this large scales exactly
            // the same as one a few digits shorter would.
            if (exponent < 1000000)
                exponent = exponent * 10 + (*d - '0');
            d++;
        }
        if (d == firstDigit || *d != 0) {
            // "1e", "1e-", or junk after the exponent digits: not a
            // number at all, same as any other unparseable token.
            error_occurred();
            return 0;
        }
        if (expSign)
            exponent = -exponent;
        expOriginal = *expAt;
        *expAt = 0;
        haveExponent = true;

        if (*cstring == 0) {
            // "e5" alone: an exponent with no mantissa digits in front of
            // it at all. The empty-string check above already ran before
            // this truncation existed, so it never caught this case.
            *expAt = expOriginal;
            error_occurred();
            return 0;
        }
    }
#endif

    while (*cstring) {
        Byte c = *cstring++;

        if (!toDigit(&c)) {
#ifdef FLOAT_SUPPORT
            if (c == '.') { // decimal point encountered - try for mantissa
                Real f = 0;

                // A second decimal point (e.g. "1.2.3") is not a valid
                // literal. Without this check the backward scan below
                // finds *a* '.' - whichever one is closest to the end of
                // the string, not necessarily this one - and silently
                // parses a number nobody typed instead of rejecting the
                // input. cstring is already past the point just consumed,
                // so this only looks for a second one.
                for (Byte* extra = cstring; *extra; extra++) {
                    if (*extra == '.') {
                        if (expAt) *expAt = expOriginal;
                        error_occurred();
                        return n;
                    }
                }

                while (*cstring) // start at end of string to work back to decimal
                    cstring++;

                while (*--cstring != '.') {
                    c = *cstring;
                    if (!toDigit(&c)) {
                        if (expAt) *expAt = expOriginal;
                        error_occurred();
                        return n;
                    }
                    f = (f + c) / base;
                }
                f = f + whole;
                if (sign)
                    f = -f;
                if (haveExponent)
                    f = scaleByPow10(f, exponent);
                if (expAt) *expAt = expOriginal;
                return floatCell(f);
            }
#endif
#ifdef FLOAT_SUPPORT
            if (expAt) *expAt = expOriginal;
#endif
            error_occurred();
            return n;
        }
        n = n * base + c;
#ifdef FLOAT_SUPPORT
        whole = whole * base + c;
#endif
    }
    if (sign)
        n = -n;
#ifdef FLOAT_SUPPORT
    if (haveExponent) {
        // No decimal point, but "1e10" is still exponent notation, not
        // an integer - always resolves to a float, same as "1.0e10" would.
        Real f = scaleByPow10(sign ? -whole : whole, exponent);
        if (expAt) *expAt = expOriginal;
        return floatCell(f);
    }
#endif
    return n;
}

Cell stringNumber(char * cstring)
{
    Cell n;
    Byte b = base;
    bool sign = (*cstring == '-');

    if (sign)
        cstring++;
    cstring += checkBase((Byte *)cstring);
    n = signDigits((Byte *)cstring, sign);
    base = b;
    return n;
}

// control flow
void compileAhead()
{
    compileIt(&_branch);
    here();
    lit(0);
    comma();
}

void compileIf()
{
    compileIt(&_zeroBranch);
    here();
    lit(0);
    comma();
}

void compileEndif()
{
    here();
    swap();
    store();
}

void compileElse()
{
    compileAhead();
    swap();
    compileEndif();
}

void compileBegin()
{
    here();
}

void compileAgain()
{
    compileIt(&_branch);
    comma();
}

void compileWhile()
{
    compileIf();
}

void compileRepeat()
{
    swap();
    compileAgain();
    compileEndif();
}

void compileUntil()
{
    compileIt(&_zeroBranch);
    comma();
}

void compileFor()
{
    compileIt(&_tor);
    compileAhead();
    here();
}

void compileNext()
{
    swap();
    compileEndif();
    compileIt(&_minusBranch);
    comma();
}

void compileExit()
{
    lit(0);
    comma();
}

void literal(Cell n)
{
    lit(n);
    if (compiling) {
        compileIt(&_lii);
        comma();
    }
}

// strings
// parse a string optionally quoted; allow for zero length strings
char * postQuote() {
 	skip(SPACE);
    if (peek() == QUOTE) {
    	tib.in++;
    	parse(QUOTE);
    	return (char *)&hp[1];
    }
    return (char *)parseWord(SPACE);
}

void makeString(char * string) // ( - a )
{
    // string is always hp+1 here (postQuote()'s quoted-string branch
    // returns &hp[1], the count-prefixed text parse() just wrote), so
    // source and destination overlap by one byte - this is shifting the
    // string left to drop that leading count byte, in place. strncpy()
    // does not guarantee correct behavior on overlapping regions; it
    // happened to work on the embedded target's libc, but is undefined
    // behavior per the C standard and traps under a hardened libc that
    // checks for it (confirmed live: caught immediately running this
    // same source against Tea-host on macOS - see
    // AutomatedTest/tools/tea_host_cli). memmove() is the correct,
    // portable tool for an overlapping copy regardless of direction.
    memmove(hp, string, strlen(string) + 1); // remove leading count
    lit((Cell)hp);
    Cell n = strlen((char *)hp);
    allot(n + 1); // account for null terminator
    aligned();
}

void quote()
{
	if (compiling)
		compileAhead();

	makeString(postQuote());

	if (compiling) {
		swap();
		compileEndif();
		literal(ret());
	}
}

// interpreter
void quit()
{
    interpretError = 0;
    spStore();
    rpStore();
    zeroTib();
    leftBracket();
    cursorReturn();
}

void semiColon(); // defined below; needed by isCompileOnly() above its own definition

// if/else/endif/begin/again/while/repeat/until/for/next/exit and ; are all
// marked IMMEDIATE ("always execute, even when compiling" - see cli.h's
// header-bit comment) so their compileXxx()/semiColon() bodies run at
// all, including while actually compiling a definition. But "always
// execute" was never meant to license running them stand-alone, outside
// any definition: their C code unconditionally manipulates compile-time
// state - the dictionary pointer (here/comma), branch-patch addresses
// left on the data stack by a matching if/begin/for, or (semiColon)
// wordlist, which resetCli() sets to NULL between definitions. Confirmed
// live: bare `for` on an empty stack didn't underflow at all - it pushed
// two garbage addresses, because compileFor() happily wrote a branch
// target through `here`/`comma` and left unbalanced compile-time
// bookkeeping on the data stack. Bare `;` would be worse: semiColon()
// dereferences wordlist->name[0] unconditionally, which segfaults
// whenever it is NULL - i.e. whenever no `:` definition is open.
//
// There being no third header-bit state ("compile-only": run while
// compiling, error otherwise) short of repacking the header byte, this
// checks identity against the fixed, known set of functions that need
// it instead - smaller and lower-risk than changing how every word's
// name byte encodes length vs flags for the sake of eleven words that
// are not expected to grow much.
static bool isCompileOnly(vector ii) {
    return ii == compileIf || ii == compileElse || ii == compileEndif
        || ii == compileBegin || ii == compileAgain || ii == compileWhile
        || ii == compileRepeat || ii == compileUntil || ii == compileFor
        || ii == compileNext || ii == compileExit || ii == semiColon;
}

static void interpret_word() {
    if (setjmp(stack_fault)) {
        // Landed here via longjmp from push()/pop() after a stack
        // breach; resetCli() already ran and cleared the TIB, so simply
        // returning is enough - the caller's "more to interpret?" check
        // finds nothing left of the line that caused the fault.
        return;
    }
    skip(SPACE);
    if (peek() == QUOTE) {
        quote();
        return;
    }
    Byte* cstring = parseWord(SPACE);
    tcbody* t = NULL;
    Byte headbits = lookup(cstring, &t);

    if (headbits != 0 && !compiling && isCompileOnly(t->ii)) {
        // Same "report clearly, do not continue" shape as the number-
        // parse-failure path just below: message, mark the error, quit()
        // to get back to a known-good state (this also matches how a
        // real compile-only violation is conventionally reported in
        // Forth systems, rather than as a bare "not found").
        msg((char*)cstring);
        msg(" compile only, use inside : ... ; <- eh?");
        errors = interpretError = 1;
        quit();
    } else if (headbits != 0) // compiling or executing regardless of compiling
        headbits == compiling ? compileIt(t) : executeIt(t);
    else { // interpreting as a number
        Cell n = stringNumber((char *)cstring);

        if (interpretError)
            quit();
        else
            literal(n);
    }
}

void interpret() { skip(SPACE);  while (peek()) interpret_word();}

// input stream
void autoEchoOn()  { keyEcho = true; }
void autoEchoOff() { keyEcho = false; }
bool autoEchoIs()  { return keyEcho; }

void emptyKeyq() { zerobq(keyq); }

static enum {TIB_FILL, TIB_LINE} tib_state = TIB_FILL;
static Long naptime = 0;

void nap_for() { naptime = ret(); }

static void parse_keyq() {
	while (qbq(keyq)) {
		Byte key = pullbq(keyq);

		switch (key) {
		case LFEED: // ignore line feeds
			key = 0;
			break;
		case BSPACE:
		case DELETE: // backspace or delete
			if (tib.in == 0) {
				tib_state = TIB_FILL;
				key = 0;
			} else
				tib.in -= 1;
			break;
		case CRETURN:
			if (keyEcho)
				cursorReturn();
			// fall through
		case 0:
			outp = 0;
			tib_state = TIB_LINE;
			end_tib();
			return;
        case HTAB: // replace horizontal tabs
            key = SPACE;
            // fall through
 		default:
			if (ESCAPE < key && key < DELETE && tib.in < LINE_LENGTH) {
				tib.buffer[tib.in++] = key;
				tib_state = TIB_FILL;
			} else
				key = BEEP;
			break;
		}

		if (keyEcho && key)
			emitByte(key);
	}
}

void no_prompt() { lineEcho = false; }

static void interpret_one() {
    skip(SPACE);
    if (peek())
        interpret_word();
    else {
        zeroTib();
        tib_state = TIB_FILL;
        if (lineEcho)
            later(dotPrompt);
        else
        	lineEcho = true;
    }
}

bool interpreting() {
	if (tib_state == TIB_FILL)
		parse_keyq();
	if (tib_state == TIB_LINE)
		interpret_one();
	return qbq(keyq) || peek() || tib_state == TIB_LINE;
}

static enum {CLI_OFF, CLI_EXECUTING, CLI_INTERPRETING, CLI_NAPPING, CLI_RUNNING} cli_state = CLI_OFF;

static void cli() {
    if (naptime) {
        cli_state = CLI_NAPPING;
        after(msec(naptime), cli);
        naptime = 0;
    } else if (executing()) {
        cli_state = CLI_EXECUTING;
    	later(cli);
    } else if (interpreting()) {
        cli_state = CLI_INTERPRETING;
		later(cli);
    } else
        cli_state = CLI_OFF;
}

void run_cli() {
    if (cli_state == CLI_OFF) {
        cli_state = CLI_RUNNING;
        later(cli);
    }
}

void keyIn(Byte c) {
    if (c == ESCAPE)
        later(resetCli);
    else
        pushbq(c, keyq);
    now(*KeyEvent);
}

void get_key() { lit( qbq(keyq) ? pullbq(keyq) : 0 ); } // CLI tools
void ask_key() { lit(qbq(keyq)); }

void evaluate(Byte* string)
{
    zeroTib(); // clear out any network debris - assure command execution
    outp = 0;
    strncpy((char *)tib.buffer, (char *)string, LINE_LENGTH);
    tib.buffer[LINE_LENGTH] = 0;
    interpret();
    zeroTib();
}

// defining words
// make header name from null terminated string no header bits, easier searching, count becomes strlen; link;string;->:ii;list ; or string is a pointer to a string and then the header is a fixed size structure;
static void makeHeader()
{
    aligned();
    here();
    lit((Cell)wordlist);
    comma();
    wordlist = (header*)ret();
    parseWord(SPACE);
    *hp |= NAME_BITS;
    allot((*hp & ~HEADER_BITS) + 2); // header bits, trailing 0
    aligned();
}

void colon()
{
    makeHeader();
    wordlist->name[0] |= SMUDGE_BITS;
    lit((Cell)colonii);
    comma();
    rightBracket();
}

void semiColon()
{
    compileExit();
    wordlist->name[0] &= ~SMUDGE_BITS;
    leftBracket();
}

void constant() /* n -- */
{
    makeHeader();
    lit((Cell)cii);
    comma();
    comma();
}

void create()
{
    makeHeader();
    lit((Cell)vii);
    comma();
}

void variable() /* n -- */
{
    create();
    comma();
}

// Tools
static void list_dictionaries() // list words in dictionaries
{
    Byte i;
    for (i = 0; i < 3; i++) {
    	PGM_P dictionary, *dictionaries[] = { constantnames, wordnames, immediatenames };
    	Byte c;
        dictionary = dictionaries[i];
        while ((c = pgm_read_byte(dictionary++)) != 0) {
            do {
                emitByte(c);
            } while ((c = pgm_read_byte(dictionary++)) != 0);
            spaces(2);
        }
    }
}

void words() // list user words and system dictionaries
{
    header* list = wordlist;

    cursorReturn();
    while (list) {
        Byte* name = list->name;
        Byte length = *name++ & ~IMMEDIATE_BITS;

        while (length--)
            emitByte(*name++);

        spaces(2);
        list = list->list;
    }
    list_dictionaries();
}

// reset all
bool cli_is_off() { return cli_state == CLI_OFF; }

void resetCli() {
    naptime = 0;
    cli_state = CLI_OFF;
    hereSay(hpStart, hpEnd - hpStart);
    wordlist = NULL;
#ifdef FLOAT_DOUBLE
    floatReset(); // the dictionary holding pinned floats is gone
#endif
    ip = NULL;
    decimal();
    quit();
}

// results: general mechanism to pass values as results from commands
Cell result;

void setCliResult(Cell n) { result = n; }
Cell getCliResult() { return result; }

// TODO: group by function; factor out magic numbers; improve names; reduce coupling so CLI can ignore parts; static functions; float support
// forget last definition if it encroaches; for bigger test scripts. support test scripting.
// need to test at the functional level with full dictionary
// need to verify here space overflow protection; perhaps other overflow protections too

bool isCompiling()
{
	return compiling != 0;
}

void init_cli() {
	namedAction(cli);
	namedAction(dotPrompt);
    namedAction(run_cli);
    namedAction(resetCli);
    resetCli();
    print(CLI_TITLE);
    later(dotPrompt);
}
