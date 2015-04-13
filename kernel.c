/* botKernel : Mar 10, 1991  Rob Chapman */
// Copyright TimbreWorks Engineering 1990-2005

#include <stdlib.h>
#include "botkernl.h"
#include "byteq.h"
#include "kernel.h"
#include "extii.h"
#include "queue.h"
#include "localio.h"
#include "library.h"

// nameless but has a tick
Headless(LIT)
Headless(BRANCH)
Headless(ZERO_BRANCH)
Headless(MINUS_BRANCH)
Headless(TO_I)
Headless(LITERAL)
Headless(QUOTE)
Headless(COUNT)
Headless(TYPE)

/* ==== Cell ==== */

void CELL(void)  /* -- n */
{
	_CELL_;
}

void CELLS(void)  /* n -- n' */
{
	_CELLS_;
}

/* ==== Stacks ==== */

/* ==== Data stack primitives ==== */

void SWAP(void)  /* m \ n -- n \ m */
{
	_SWAP_;
}

void DUP(void)  /* m -- m \ m */
{
	_DUP_;
}

void DROP(void)  /* m -- */
{
	_DROP_;
}

/* ==== Return stack primitives ==== */

void TO_R(void)  /* m -- */
{
	_TOR_;
}

void R(void)  /* -- m */
{
	_R_;
}

void R_FROM(void)  /* -- m */
{
	_RFROM_;
}

/* ==== Stack control ==== */
Cell *rp0_=0;

void rp_0(void)  /* -- a */  /* location of bottom of return stack */
{
	lit(&rp0_);
}

Cell *sp0_=0;

void sp_0(void)  /* -- a */  /* location of bottom of parameter stack */
{
	lit(&sp0_);
}


void RP_STORE(void)  /* -- */
{
	rp = rp0_;
}

void SP_STORE(void)  /* ? -- */
{
	sp=sp0_;
}

void DEPTH(void)  /* -- n */
{
	*--rp = (Cell)(sp0_ - sp);
	*--sp = *rp++;
}

/* ==== Stack macros ==== */

void NUP(void)  /* m \ n -- m \ m \ n */
{
	_NUP_;
}

void NIP(void)  /* m \ n -- n */
{
	_NIP_;
}

void OVER(void)  /* m \ n -- m \ n \ m */
{
	_OVER_;
}

void TUCK(void)  /* m \ n -- n \ m \ n */
{
	_TUCK_;
}

void TWO_DUP(void)  /* m \ n -- m \ n \ m \ n */
{
	_2DUP_;
}

void TWO_DROP(void)  /* m \ n -- */
{
	_2DROP_;
}

void QUESTION_DUP(void)  /* n -- [n] \ n */
{
	if(_TOP_)
	{	  /* DUP IF */
		DUP();
	}
}

/* ==== Execution of the VFM ==== */

void EXECUTE(void)  /* a -- */
{
	_EXECUTE_;
}

/* ==== ALU ==== */

/* ==== Logic ==== */

void AND(void)  /* m \ n -- p */
{
	_AND_;
}

void OR(void)  /* m \ n -- p */
{
	_OR_;
}

void XOR(void)  /* m \ n -- p */
{
	_XOR_;
}

void NOT(void)  /* m -- n */
{
	_NOT_;
}

/* ==== Shift ==== */

void TWO_STAR(void)  /* m -- n */
{
	_2STAR_;
}

void TWO_SLASH(void)  /* m -- n */
{
	_2SLASH_;
}

void U_2_SLASH(void)  /* m -- n */
{
	_U2SLASH_;
}

/* ==== Math ==== */

void PLUS(void)  /* m \ n -- p */
{
	_PLUS_;
}

void MINUS(void)  /* m \ n -- p */
{
	_MINUS_;
}

void NEGATE(void)  /* m -- n */
{
	_NEGATE_;
}

/* ==== signed multiply and unsigned divide ==== */

void SLASH_MOD(void)  /* n \ m -- remainder \ quotient */
{
	_SLASHMOD_;
}

void SLASH(void)  /* n \ m -- quotient */
{
	_SLASH_;
}

void MOD(void)  /* n \ m -- remainder */
{
	_MOD_;
}

void STAR(void)  /* n \ m -- p */
{
	_STAR_;
}

/* ==== Compare ==== */

void EQUALS(void)  /* n \ m -- flag */
{
	_EQUALS_;
}

void LESS_THAN(void)  /* n \ m -- flag */
{
	_LESSTHAN_;
}

void GREATER_THAN(void)  /* n \ m -- flag */
{
	_GREATERTHAN_;
}

void U_LESS_THAN(void)  /* n \ m -- flag */
{
	_ULESSTHAN_;
}

void U_GREATER_THAN(void)  /* n \ m -- flag */
{
	_UGREATERTHAN_;
}

/* ==== Absolute, MinMax ==== */

void ABS(void)  /* n -- n */
{
	_ABS_;
}

void MAX(void)  /* n \ m -- p */
{
	_MAX_;
}

void MIN(void)  /* n \ m -- p */
{
	_MIN_;
}

/* ==== Memory ==== */

/* ==== Cells ==== */

void FETCH(void)  /* a -- n */
{
	_FETCH_;
}

void STORE(void)  /* n \ a -- */
{
	_STORE_;
}

void FETCH_PLUS(void)  /* a -- n \ a+ */
{
	--sp, _TOP_ = _NEXT_ + sizeof(Cell);
	_NEXT_ = *(Cell *)_NEXT_;
}

void STORE_PLUS(void)  /* n \ a -- a+ */
{
	*(Cell *)_TOP_ = _NEXT_;
	_NEXT_ = _TOP_ + sizeof(Cell), _DROP_;
}

void FETCH_MINUS(void)  /* a -- n \ a- */
{
	--sp, _TOP_ = _NEXT_ - sizeof(Cell);
	_NEXT_ = *(Cell *)_NEXT_;
}

/* ==== Bytes ==== */

void C_FETCH(void)  /* a -- c */
{
	_CFETCH_;
}

void C_STORE(void)  /* c \ a -- */
{
	_CSTORE_;
}

void C_FETCH_PLUS(void)  /* a -- c \ a+ */
{
	--sp, _TOP_ = _NEXT_ + 1;
	_NEXT_ = (Cell)(*(Byte *)_NEXT_);
}

void C_STORE_PLUS(void)  /* c \ a -- a+ */
{
	*(Byte *)_TOP_ = (Byte)_NEXT_;
	_NEXT_ = _TOP_ + 1, _DROP_;
}

void C_PLUS_STORE(void)  /* c \ addr -- */
{
	*(Byte *)(_TOP_)+=(Integer)_NEXT_;
	_2DROP_;     /* C+! */
}

/* ==== Bits ==== */

void PLUS_BITS(void)  /* bits \ addr -- */
{
	*(Byte *)_TOP_ |= _NEXT_;
	sp += 2;
}

void MINUS_BITS(void)  /* bits \ addr -- */
{
	*(Byte *)_TOP_ = (Byte)(*(Byte *)_TOP_ & ~_NEXT_);
	sp += 2;
}

/* ==== Longs ==== */
void longFetch(void) // a - lo \ hi
{
	Long l = *(Long *)sp[0];
	
	*sp-- = (Cell)(l & 0xFFFF);
	*sp = (Cell)(l>>16);
}

void longStore(void) // lo \ hi \ a -
{
	*(Long *)sp[0] = ((Long)sp[1]<<16) + sp[2];
	sp += 3;
}
	
/* ==== Shorts ==== */
void shortFetch(void) // a - n
{
	Short l = *(Short *)sp[0];
	
	sp[0] = (Cell)(l & 0xFFFF);
}

void shortStore(void) // n \ a -
{
	*(Short *)sp[0] = (Short)sp[1];
	sp += 2;
}
	
/* ==== Queues == | insert | remove | end |  data space for circular queue | */
void ZERO_Q(void) /* q -- */  /* initialize a queue */
{
	zeroq((Qtype *)*_DROP_);
}

void Q(void)  /* q -- n */
{
	_TOP_ = q((Qtype *)_TOP_);
}

void Q_QUERY(void) /* q -- n */  /* return the size of a queue */
{
	_TOP_ = queryq((Qtype *)_TOP_);
}

void PULL(void) /* q -- n */  /* |queued item|>s  s is transferred to the data stack */
{
	_TOP_ = pullq((Qtype *)_TOP_);
}

void PUSH(void) /* n \ q -- */  /* q>|ueued items|  q is transferred from the data stack */
{
	pushq(_NEXT_,(Qtype *)_TOP_);
	_2DROP_;
}

/* ==== Memory tools ==== */

void BL(void)  /* -- n */
{
	lit(32);
}

void CMOVE(void)  /* src \ dest \ count -- */
{
	memcpy((void *)_NEXT_, (void *)_TOS_, _TOP_);
	sp+=3;
}

void LESS_THAN_CMOVE(void)  /* src \ dest \ count -- */
{
	memmove((void *)_NEXT_, (void *)_TOS_, _TOP_);
	sp+=3;
}

void MOVE(void)  /* src \ dest \ count -- */
{
	memcpy((void *)_NEXT_, (void *)_TOS_, _TOP_*sizeof(Cell));
	sp+=3;
}

void FILL(void)  /* addr \ count \ char -- */
{
	memset((void *)_TOS_, (Byte)_TOP_, _NEXT_);
	sp+=3;
}

void ERASE(void)  /* addr \ count -- */
{
	lit(0);
	FILL();
}

void DIFFER_QUERY(void)  /* a \ a -- a+ \ a+ \ bits */
{
	--sp;
	_TOP_ = (Cell)(*(Byte *)_NEXT_++ ^ *(Byte *)_TOS_++);
}

void BITS_DIFFER_QUERY(void)  /* a \ a \ mask -- a+ \ a+ \ bits */
{
	_TOP_ &= (Cell)(*(Byte *)_NEXT_++ ^ *(Byte *)_TOS_++);
}

void MEMCMP(void)  /* a \ b \ length -- flag:[ -c 0 +c ]  */
{
	_TOS_ = (Cell)memcmp((void *)_TOS_, (void *)_NEXT_, _TOP_);
	sp += 2;
}

void SAME_QUERY(void)  /* a \ b \ length -- flag */
{
	MEMCMP();
	_0EQUALS_;
}

void COMPARE(void)  /* a \ b -- f */
{
	DUP();
	C_FETCH();
	_TOP_+=1;	/* + */
	SAME_QUERY();
}

/* ==== Dictionary ==== */

/* ==== Memory Management ==== */
Cell dp_=0;

void dp(void)  /* -- a */  /* end of dictionary and start of free memory */
{
	lit(&dp_);
}

void HERE(void)  /* -- addr */
{
	dp();
	FETCH();
}

void ALLOT(void)  /* n -- */
{
	dp();
	_PLUSSTORE_;
}

/*  char-align#  TUCK +  SWAP  NOT AND */

void ALIGN(void)  /* a -- a' */
{
	struct{void*x;char y;void*z;}align;
	Cell z = (Cell)&align.z - (Cell)&align.y - sizeof(char); /* 1 or 3 */
	
	*sp = (*sp + z) & ~z;
}

void aligned(void)  /* -- */
{
	HERE();
	ALIGN();
	HERE();
	MINUS();
	ALLOT();
}

void C_COMMA(void)  /* n -- */
{
	HERE();
	C_STORE();
	lit(1);
	ALLOT();
}

void COMMA(void)  /* n -- */
{
	aligned();
	HERE();
	STORE();
	_CELL_;
	ALLOT();
}

/* ==== Headers ==== | link | name | tick | */
void L_TO_NAME(void)  /* link -- name */
{
	_TOP_+=sizeof(Cell);	  /* CELL + */
}

void N_TO_TICK(void)  /* name -- tick */
{
	C_FETCH_PLUS();
	SWAP();
	_TOP_&=31;     /* AND */
	_PLUS_;
	ALIGN();
}

void L_TO_TICK(void)  /* link -- tick */
{
	L_TO_NAME();
	N_TO_TICK();
}

void L_TO_LINK(void)  /* link -- previous */
{
	if(_TOP_)
		FETCH();
}

/* ==== Dictionary Searching ==== */
Cell current;

void latest(void)  /* -- a */  /* point to latest dictionary entry */
{
	lit(current);
}

void NAME_QUERY(void)  /* string \ name -- flag */
{
	lit(0x3F);
	BITS_DIFFER_QUERY();
	if(*_DROP_)   /* IF */
	{
		sp++;
		*sp=0;
		return;	 /* EXIT */
	}
	--sp;
	*sp = (*(Byte *)(sp[1]-1)) & 0x1fU;
	SAME_QUERY();
}

void SEARCH_DICTIONARY(void)  /* string \ list -- link \ yes | -- string \ no */
{
	for(;;)       /* BEGIN */
	{  /* search dictionary link list */
		L_TO_LINK();
		if(!_TOP_) break;      /* DUP WHILE */
		TWO_DUP();
		L_TO_NAME();
		NAME_QUERY();
		if(*_DROP_)	 /* IF */
		{
			_NIP_;
			lit(-1);
			return;	  /* EXIT */
		}
	};	 /* REPEAT */
}

void search_context(void)  /* s -- a \ f */
{
	latest(), SEARCH_DICTIONARY();
}

/* Environment */
/* ==== Output stream ==== */
BQUEUE(160,eq);

Cell out_=0;

void out(void)  /* -- a */  /* absolute cursor position since last cursor return */
{
	lit(&out_);
}

void EMIT(void)  /* char -- */
{
	Byte c = (Byte)*sp++;

	if ((c == 8) || (c == 127))       /* backspace? or delete */
		out_ -= 1, c = 8;
	else if (c == 13)  /* cursor return? */
		out_ = 0;
	else if ((c != 10) && (c != 7))
		out_ += 1;         /* not line feed, pacing character or bell? */
	safe_emit(c);
}

/* ==== Output formatter ==== */

void _CR(void)  /* -- */
{
	lit(10);
	EMIT();
	lit(13);
	EMIT();
}

void QUESTION_CR(void)  /* -- */
{
	out();
	FETCH();
	if(*_DROP_)       /* IF */
		_CR();
}

void SPACE(void)  /* -- */
{
	BL();
	EMIT();
}

void SPACES(void)  /* n -- */
{
	Byte n = (Byte)*sp++;
	
	if (n > 80)
		return;
	while(n--)
		SPACE();
}

void COUNT(void)  /* addr -- addr' \ count */
{
	C_FETCH_PLUS();
	SWAP();
}

void TYPE(void)  /* addr \ count -- */
{
	Byte n = (Byte)*sp++, *a = (Byte *)*sp++;
	
	while (n--)
		lit(*a++), EMIT();
}
// for c@+ swap emit next drop

/* ==== Number Output ==== */
Byte base_=10;

void base(void)  /* -- a */
{
	lit(&base_);
}


void HEX(void)  /* -- */
{
	base_ = 16;
}

void DECIMAL(void)  /* -- */
{
	base_ = 10;
}

void PAD(void)  /* -- addr */
{
	HERE();
	_TOP_+=LINELENGTH;	 /* + */
	ALIGN();
}

void HOLD(void)  /* char -- */
{
	lit(-1);
	PAD();
	_PLUSSTORE_;
	PAD();
	FETCH();
	C_STORE();
}

void START_NUMBER_CONVERSION(void)  /* -- */
{
	PAD();
	DUP();
	STORE();
}

void CONVERT_DIGIT(void)  /* n -- n */
{
	Byte c = (Byte)(sp[0]%base_);

	sp[0] /= base_;
	if (c > 9)
		c += 7;
	c += 48;
	lit(c), HOLD();
}
/* base @ /mod swap 9 over < if 7 + then 48 + hold */

void CONVERT_NUMBER(void)  /* n -- n */
{
	for(;;)	/* BEGIN */
	{
		CONVERT_DIGIT();
		if(*sp == 0)break;	/* 0= UNTIL */
	}
}

void SIGN(void)  /* m \ n -- n */
{
	if((Integer)_NEXT_ < 0)      /* IF */
	{
		lit(45);
		HOLD();
	}
	sp[1] = sp[0];
	sp++;
}

void END_NUMBER_CONVERSION(void)  /* n -- addr \ count */
{
	_DROP_;
	PAD();
	FETCH();
	PAD();
	OVER();
	MINUS();
}

void DOT_R(void)  /* n \ m -- */
{
	_TOR_;
	START_NUMBER_CONVERSION();
	if(base_==10)	 /* IF */
	{
		DUP();
		ABS();
		CONVERT_NUMBER();
		SIGN();
	}
	else      /* ELSE */
	{
		CONVERT_NUMBER();
	}
	END_NUMBER_CONVERSION();
	_RFROM_;
	OVER();
	MINUS();
	SPACES();
	TYPE();
}

void DOT(void)  /* n -- */
{
	lit(0);
	DOT_R();
	SPACE();
}

/* ==== Input stream ==== */
BQUEUE(80,kq);

void KEY_QUERY(void)  /* -- flag */
{
	lit(qbq(kq));
}

void KEY(void)  /* -- char */
{
	for(;;)    /* BEGIN */
	{
		KEY_QUERY();
		if(*_DROP_)break;       /* 0= WHILE */
	};	 /* REPEAT */
	lit(pullbq(kq));
}

/* ==== Text input buffer ==== */
Cell in_=0;

void in(void)  /* -- a */  /* buffer index */
{
	lit(&in_);
}

struct
{
	Cell name8;
	char name9[LINELENGTH + 1];
	/* pointer; terminal input buffer */
}tib_;

void tib(void)  /* -- a */  
{
	lit(&tib_);
}

void INPUT(void)  /* -- addr */
{
	*--sp = tib_.name8 + in_;
}

void ZERO_TIB(void)  /* -- */
{
	in_ = 0;
	*--sp = 0;
	INPUT();
	C_STORE();
}

void PLUS_IN(void)  /* addr -- */
{
	in_ = *_DROP_ - tib_.name8;
}

/* ==== Input scanner ==== */

void SKIP(void)  /* char -- */
{
	Byte c = (Byte)*sp++, *input = (Byte *)(tib_.name8 + in_);

	while ((*input) && (*input == c))
		input++;
	in_ = (Cell)(input - tib_.name8);
}
/* >r input begin c@+ swap ?dup if r - else -1 then until 1 - +in r> drop */

void SCAN(void)  /* char -- */ // scan input for a character or 0
{
	Byte c = (Byte)*sp++, *input = (Byte *)(tib_.name8 + in_);

	while (*input)
		if (*input++ == c)
			break;
	in_ = (Cell)(input - tib_.name8);
}
/* >r input begin c@+ swap ?dup if r = else 1- -1 then until +in r> drop */

void PARSE(void)  /* char -- */ // parse string till char or 0 from input to here count prefixed
{
	Byte b, c = (Byte)*sp++, n=0, *input = (Byte *)(tib_.name8 + in_), *s = (Byte *)dp_;
	
	while (*input)
	{
		if ((b = *input++) == c)
			break;
		s[++n] = b;
	}
	*s = n;
	in_ = (Cell)(input - tib_.name8);
}
/*
>r here dup 1+ dup input
begin c@+ over ?dup if r xor else 1- 0 then while >r swap c!+ r> repeat
+in drop swap - swap c! r> drop
*/
/* ==== Error recovery ==== */
//extern jmp_buf env;
Byte interpretError;

void ABORT(void)  /* -- */
{
	interpretError = 1;
}

void error(void)  /* -- */
{
	HERE();
	COUNT();
	TYPE();
//	msg("\006<- eh?");
	msg("<- eh?");
	ABORT();
}

void QUESTION_ERROR(void)  /* flag -- */
{
	if(*_DROP_)	 /* IF */
		error();
}

/* ==== Number Conversion ==== */
void NEGATIVE_QUERY(void)  /* a -- f */
{
	*sp = (Cell)(*(Byte *)*sp == '-');
}
/* C@ 45 = */

void PREFIXED_QUERY(void)  /* a \ c -- f */
{
	Byte f = (Byte)*sp++;

	*sp = ((*(Byte *)*sp == '0') && (f > 2));
}
/* 2 > swap c@ 48 = and */

void SKIP_PREFIX(void)  /* a \ c \ n -- a' \ c' */
{
	sp[2] += sp[0];
	sp[1] -= sp[0];
	sp++;
}
/* tuck - >r + r> */

void LOWERCASE(void)  /* c -- c' */
{
	if (sp[0] >= 'A' && sp[0] <= 'Z')
		sp[0] ^= 'A'^'a';
}

void BIN(void)  /* -- */
{
	base_ = 2;
}

void SET_BASE(void)  /* a \ c -- a' \ c' */
{  /* check for prefixes: 0X, 0x, 0C, 0c, 0B or 0b */
	if (_TOP_ > 2)
		if (*(Byte *)_NEXT_ == '0') // count is longer than 2 and first digit is 0
		{
			switch(*(Byte*)(_NEXT_+1))
			{
				case 'b': case 'B':	BIN(); break;
				case 'c': case 'C':	OCT(); break;
				case 'x': case 'X':	HEX(); break;
			}
			_TOP_ -= 2;
			_NEXT_ += 2;
		}
}

Byte toDigit(Byte *n) // convert character to number according to base
{// covers all alphanumerics and bases
	Byte c = (Byte)(*n - '0');

	if (c > 9)
	{
		c -= 7;
		if (c > 35)
			c -= ' ';
		if (c < 10)
			return 0;
	}
	*n = c;
	return (Byte)(c < base_);
}

void DIGIT(void)  /* char -- n \ flag */
{
	Byte flag = toDigit((Byte *)&sp[0]);

	lit(flag);
}

void DIGITS_QUERY(void)  /* a \ c -- f */
{
	--sp,sp[0]=sp[1];	/* { DUP }*/
	lit(1);
	if((Integer)sp[1]<(Integer)sp[0]) *++sp=(Long)-1; else *++sp=0;	/* { < }*/
	if(*sp++)	/* IF */
	{
		sp+=2;	/* { 2DROP }*/
		lit(0);	/* { NO }*/
		return;	/* EXIT */
	}
	lit(-1);	/* { YES }*/
	*--rp=sp[1],sp[1]=sp[0],sp[0]=*rp++;	/* { SWAP }*/
	for(*--rp=*sp++;*rp;(*rp)--)	/* FOR */
	{
		*--rp=*sp++;	/* { >R }*/
		C_FETCH_PLUS();
		*--rp=sp[1],sp[1]=sp[0],sp[0]=*rp++;	/* { SWAP }*/
		DIGIT();
		sp[1]=sp[0],sp++;	/* { NIP }*/
		lit(*rp++);	/* { R> }*/
		sp[1]&=sp[0],sp++;	/* { AND }*/
	}	/* NEXT */
	rp++;
	sp[1]=sp[0],sp++;	/* { NIP }*/
}

void NUMBER_QUERY(void)  /* string -- f */
{
	base();
	sp[0]=*(Cell *)sp[0];	/* { @ }*/
	*--rp=*sp++;	/* { >R }*/
	COUNT();
	--sp,sp[0]=sp[2];	/* { OVER }*/
	NEGATIVE_QUERY();
	if(*sp++)	/* IF */
	{
		lit(1);
		SKIP_PREFIX();
	}
	SET_BASE();
	DIGITS_QUERY();
	lit(*rp++);	/* { R> }*/
	base();
	*(Cell *)sp[0]=sp[1],sp+=2;	/* { ! }*/
}

void DIGITS(void)  /* a \ c -- n */ // convert string to number according to base
{
	Byte n = (Byte)*sp++, *a = (Byte *)*sp, m;

	if ((signed char)n < 1)
	{
		error();
		return;
	}
	
	*sp = 0;
	while (n--)
	{
		m = *a++;
		if (!toDigit(&m))
		{
			error();
			return;
		}
		*sp = *sp*base_ + m;
	}
}

/*
dup 1 < ?error
interpreterError @ if exit then
0 swap for >r c@+ swap digit 0= ?error interpretError @ if exit endif r> base @ * + next
nip */

void NUMBER(void)  /* string -- n */
{
	Byte b = base_, f;

	COUNT();
	if ((f = (Byte) (*(Byte *)sp[1] == '-') ) != 0)
		sp[1] += 1, sp[0] -= 1;
	SET_BASE();
	DIGITS();
	if (interpretError) return;
	if (f)
		sp[0] = -sp[0];
	base_ = b;
}
/* base @ >r
   count over -? dup >r
   if 1 skipPrefix then
   setBase digits interpretError @ if exit then
   r> if negate
   r> base !
*/

/* ==== Compiler ==== */
Cell compile_=0;

void compile(void)  /* -- a */  /* state of interpreting/ compiling */
{
	lit(&compile_);
}


void RIGHT_SQUARE_BRACKET(void)  /* -- */
{
	compile_ = 0x80;
}

void LEFT_SQUARE_BRACKET(void)  /* -- */
{
	compile_ = 0;
}

void compileIt(Cell tick)  /* tick -- */
{
	lit(tick);
	COMMA();
}

/* ==== Inner interpreters for cells ==== */

void TO_I(void)  /* n -- */
{
	*--rp=*_DROP_;
}

void I_FROM(void)  /* -- n */
{
	lit(*rp++);
}

void LIT(void)  /* -- n */
{
	lit(*ip++);
}

void BRANCH(void)  /* -- */
{	
	ip = (void (***)())(*ip);
}

void ZERO_BRANCH(void)  /* f -- */
{
	if(!*_DROP_)	  /* IF */
		ip = (void (***)())(*ip);
	else	 /* ELSE */
		ip++;
}

void MINUS_BRANCH(void)  /* -- */
{
	if(*rp)       /* IF */
	{
		*rp-=1;      /* 1 - */
		ip = (void (***)())(*ip);  /* branch back */
	}
	else	 /* ELSE */
	{
		rp++;  /* drop count */
		ip++;  /* skip over */
	}
}

/* ==== Outer interpreter for text ==== */

void COMMENT(void)  /* -- */  /* Scan for multi line comments*/
{
	lit(41);
	SCAN();
}

void WORD(void)  /* char -- */
{
	DUP();
	SKIP();
	PARSE();
}

void CHAR(void)  /* -- c */
{
	BL();
	SKIP();
	INPUT();
	C_FETCH();
}

void FIND_QUERY(void)  /* -- tick \ status | -- string \ no */
{
	BL();
	WORD();
	HERE();
	search_context();
	if(_TOP_)
	{	 /* DUP IF */
		_DROP_;
		DUP();
		L_TO_TICK();
		SWAP();
		L_TO_NAME();
		C_FETCH();
		_TOP_&=224;      /* AND */
		return;
	}
	_DROP_;
	search_dictionaries(); // this needs to come last to respect search order
}

void QUESTION_FIND(void)  /* -- tick */
{
	FIND_QUERY();
	_0EQUALS_;
	QUESTION_ERROR();
}

void QUESTION_EXECUTE(void)  /* tick \ status -- */
{
	if(compile_ == *sp++)
		compileIt(*sp++);
	else
		EXECUTE();
}

void LITERAL(void)  /* n -- [n] */
{
	if (compile_)       /* IF */
	{
		compileIt((Cell)&_LIT.tick);      /* ' */
		COMMA();
	}
}

//jmp_buf env;

void INTERPRET(void)  /* -- */
{
	for(;;)   /* BEGIN */
	{
		CHAR();
		if(!*_DROP_)break;	  /* WHILE */
		FIND_QUERY();
		QUESTION_DUP();
		if(*_DROP_)     /* IF */
			QUESTION_EXECUTE();
		else  /* ELSE */
		{
			NUMBER();
			if (interpretError)
			{
				QUIT();
				break;
			}
			LITERAL();
		}
	};      /* REPEAT */
}

/* ==== Key collector ==== */
#ifndef PROMPTSTRING  // replace by defining
#define PROMPTSTRING "\010timbre: "
#endif

Byte prompt_[10]={PROMPTSTRING};

void prompt(void)  /* -- a */  /* contains count prefixed string for prompt */
{
	lit(&prompt_);
}

void setPrompt(char *string)
{
	prompt_[0] = (Byte)strlen(string);
	strcpy((char*)&prompt_[1], string);
}

void DOT_PROMPT(void)  /* -- */
{
	QUESTION_CR();
	if (compile_)
		lit("\002] ");
	else
		prompt();
	COUNT();
	TYPE();
}

// keyEcho suppresses key echoing for a line; autoecho suppresses always
Byte keyEcho = 0, autoecho = 0; // can be turned off to silently process a line

void autoEchoOn(void) // echo keys back
{
	keyEcho = autoecho = 1;
}

void autoEchoOff(void) // don't echo keys back
{
	keyEcho = autoecho = 0;
}

void COLLECT(void)  /* -- */
{
	Byte k = keyEcho;

	KEY();
	if( _TOP_ == 8 || _TOP_ == 127 )  // backspace or delete
	{
		if(in_ != 0)
			in_ -= 1;
		else
			_TOP_ = 7;
	}
	else if( _TOP_ == 10 )
	{
		_DROP_;
		return;
	}
	else if( _TOP_ == 13 || _TOP_ == 0)  /* IF it is a cursor return */
	{
		keyEcho = autoecho;
		_TOP_ = 0;
		INPUT();
		C_STORE();
		out_ = 0;
		if (k)
			SPACE();
		in_ = 0;
		INTERPRET();
		ZERO_TIB();
		DOT_PROMPT();
		return;
	}
	else if( _TOP_ < 27 )
		_TOP_ = 7;
	else if( in_ < 78 ) // check in not out!
	{
		DUP();
		INPUT();
		C_STORE_PLUS();
		PLUS_IN();
	}
	else
		_TOP_ = 7;
	if (keyEcho)
		EMIT();
	else
		_DROP_;
}

void COLLECTOR(void)  /* -- */
{
	if(qbq(kq))   /* IF */
		COLLECT();
}

void INIT(void)  /* -- */
{
	zerobq(kq);
	zerobq(eq);
	tib();
	DUP();
	_TOP_+=sizeof(Cell);    /* CELL + */
	SWAP();
	STORE();
}

/* ==== Control ==== */

void QUIT(void)  /* -- */
{
	interpretError = 0;
	sp = sp0_;	/* SP! */
	rp = rp0_;	/* RP! */
	ZERO_TIB();
	LEFT_SQUARE_BRACKET();
	_CR();
	DOT_PROMPT();
}

/* ==== Compilers ==== */

/* ==== Control flow ==== */

void AHEAD(void)  /* -- a */
{
	compileIt((Cell)&_BRANCH.tick);
	HERE();
	lit(0);
	COMMA();
}

void IF(void)  /* -- a */
{
	compileIt((Cell)&_ZERO_BRANCH.tick);
	HERE();
	lit(0);
	COMMA();
}

void ENDIF(void)  /* a -- */
{
	HERE();
	SWAP();
	STORE();
}

void ELSE(void)  /* a -- a */
{
	AHEAD();
	SWAP();
	ENDIF();
}

void BEGIN(void)  /* -- m */
{
	HERE();
}

void AGAIN(void)  /* m -- */
{
	compileIt((Cell)&_BRANCH.tick);
	COMMA();
}

void WHILE(void)  /* m -- m \ n */
{
	IF();
}

void REPEAT(void)  /* m \ n -- */
{
	SWAP();
	AGAIN();
	ENDIF();
}

void UNTIL(void)  /* m -- */
{
	compileIt((Cell)&_ZERO_BRANCH.tick);
	COMMA();
}

void FOR(void)  /* -- m \ n */
{
	compileIt((Cell)&_TO_I.tick);
	AHEAD();
	HERE();
}

void NEXT(void)  /* m \ n -- */
{
	SWAP();
	ENDIF();
	compileIt((Cell)&_MINUS_BRANCH.tick);
	COMMA();
}

void EXIT(void)  /* -- */
{
	lit(0);
	COMMA();
}

void TICK(void)  /* -- tick */
{
	QUESTION_FIND();
	if (interpretError)
	{
		QUIT();
		return;
	}
	if (compile_)       /* IF */
		LITERAL();
}

/* ==== Strings ==== */

void STRING(void)  /* terminator -- string */
{
	PARSE();
	HERE();
	DUP();
	C_FETCH();
	_TOP_+=1;	/* + */
	ALLOT();
	aligned();
}

void QUOTE(void)  /* -- s */
{
	if (compile_)       /* IF */
		AHEAD();
	lit(34);
	STRING();
	if (compile_)       /* IF */
	{
		SWAP();
		ENDIF();
		LITERAL();
	}
}
// compile @ if ahead then 34 string compile @ if swap endif literal then

void DOT_QUOTE(void)  /* -- */
{
	if (compile_)       /* IF */
	{
		QUOTE();
		compileIt((Cell)&_COUNT.tick);
		compileIt((Cell)&_TYPE.tick);
	}
	else       /* ELSE */
	{
		lit(34);
		WORD();
		HERE();
		COUNT();
		TYPE();
	}
}

/* ==== Defining words ==== */
void MAKE_LINK(void)  /* current -- */
{
	latest();
	FETCH();
	COMMA();
	latest();
	STORE();
}

void MAKE_NAME(void)  /* -- */
{
	lit(128);
	HERE();
	DUP();
	C_FETCH();
	_TOP_+=1;	/* + */
	ALLOT();
	PLUS_BITS();
}

void HEADER(void)  /* -- */
{
	aligned();
	HERE();
	MAKE_LINK();
	BL();
	WORD();
	MAKE_NAME();
	aligned();
}

void SMUDGE(void)  /* -- */
{
	lit(32);
	latest();
	L_TO_LINK();
	L_TO_NAME();
	PLUS_BITS();
}

void RECURSIVE(void)  /* -- */
{
	lit(32);
	latest();
	L_TO_LINK();
	L_TO_NAME();
	MINUS_BITS();
}

void COLON(void)  /* -- */
{
	HEADER();
	SMUDGE();
	lit(COLON_II);
	COMMA();
	RIGHT_SQUARE_BRACKET();
}

void SEMI_COLON(void)  /* -- */
{
	EXIT();
	RECURSIVE();
	LEFT_SQUARE_BRACKET();
}

void CONSTANT(void)  /* n -- */
{
	HEADER();
	lit(CII);
	COMMA();
	COMMA();
}

void CREATE(void)  /* -- */
{
	HEADER();
	lit(VII);
	COMMA();
}

void VARIABLE(void)  /* n -- */
{
	CREATE();
	COMMA();
}

/* ==== End of Kernel ==== */

Cell _listhead = 0;

void RESET_KERNEL(void)  /* -- */
{
	rp = rp0_;	/* RP! */
	sp = sp0_;	/* SP! */
	current = (Cell)&_listhead;
	INIT();
	QUIT();
}

