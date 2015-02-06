/* Code Libraries for botKernel */
// Copyright TimbreWorks Engineering 1990-2005

#include <stdlib.h>
#include "botkernl.h"
#include "byteq.h"
#include "queue.h"
#include "kernel.h"
#include "library.h"
#include "localio.h"
#include "library.h"

extern Byte kq[];
extern Byte base_;

// for dictionaries
extern void (*wordbodies[])();
extern void (*constantbodies[])();
extern void (*immediatebodies[])();
extern PROGMEM char wordnames[];
extern PROGMEM char constantnames[];
extern PROGMEM char immediatenames[];

/* ==== botKernel addendums ==== */

void OCT(void)  /* -- */
{
	base_ = 8;
}

void SHIFT(void)  /* n \ m -- n */
{
	if ((signed)_TOP_ < 0)
		_NEXT_ >>= abs((Integer)_TOP_);
	else
		_NEXT_ <<= _TOP_;
	_DROP_;
}

void ZERO_KEYS(void)  /* -- */
{
	zerobq(kq);
}

void BACK_TICK(void)  /* -- n */
{
	BL();
	WORD();
	HERE();
	*sp+=1;	  /* + */
	C_FETCH();
	LITERAL();    /* ' */
}

void POP(void) /* q -- n */  /* q<|ueued items|  q is transferred to the data stack */
{
	_TOP_ = popq((Queue_t)_TOP_);
}

void STUFF(void) /* n \ q -- */  /* |queued item|<s s is transferred from the data stack */
{
	stuffq(_NEXT_,(Queue_t)_TOP_);
	_2DROP_;
}

void P(void)  /* q -- n */
{
	_TOP_ = p((Queue_t)_TOP_);
}

// interpret a C string; no output other than generated output
extern Byte keyEcho;
Cell droppedInput;
Byte kqleft(void);
void ZERO_TIB(void);

void evaluate(Byte *string)
{
	Byte length	= (Byte)strlen((char *)string);

	if (kqleft() < length)
	{
		droppedInput++;
		return;
	}
	keyEcho = 0; // turn off key echo
	ZERO_TIB(); // clear out any network debris - assure command execution
	while (length--)
		keyin(*string++);
	keyin(0); // append an non echo carriage return to interpret it
}	

/* ==== String support ==== */
struct
{
	Byte name8[20];
}number_;

void number(void)  /* -- a */  /* space for writing a number string */
{
	*--sp=(Cell)&number_;
}

void DOLLARS_STORE(void)  /* s \ a -- */
{
	--sp,sp[0]=*(Byte*)sp[2];	/* OVER C@ */
	sp[0]+=1;	/* + */
	CMOVE();
}

void PLUS_DOLLARS(void)  /* s \ a \ n -- */
{
	_TOR_;
	OVER();
	COUNT();
	PLUS();
	_R_;
	CMOVE();
	_RFROM_;
	SWAP();
	C_PLUS_STORE();
}

void NUMBER_TO_DOLLARS(void)  /* n -- s */
{
	START_NUMBER_CONVERSION();
	CONVERT_NUMBER();
	END_NUMBER_CONVERSION();
	DUP();
	number();
	C_STORE_PLUS();
	SWAP();
	CMOVE();
	number();
}

void DOLLARS_WITHIN(void)  /* s1 \ a \ n -- a | 0 */
{  /* address of string s1 within or 0  Note: strings cannot contain nulls */
	TUCK(), HERE(), *sp += 256,  SWAP(), CMOVE();
	HERE(), *sp += 256,  PLUS(), *--sp = 0, SWAP(), C_STORE();
	C_FETCH_PLUS(), OVER(), HERE(), SWAP(), CMOVE();
	HERE(), PLUS(), *--sp = 0, SWAP(), C_STORE();
	HERE(),  DUP(), *sp += 256;
	sp[1] = (Cell)strstr((void *)sp[0], (void *)sp[1]), sp++;
}

/* ==== Fancy number output ==== */

void DOT_B(void)  /* n -- */ // print bits in number
{
	Cell n = *sp++, mask = ~(((Cell)-1)>>1);
	Byte i;
	
	SPACE();
	// skip upper bytes if all zero
	for (i = (sizeof(Cell) - 1)*8; i > 0; i -= 8)
		if (((0xff<<i) & n) == 0)
			mask >>= 8;
		else
			break;
		
	while (mask)
	{
		for (i=8; i; i--)
		{
			if (n&mask)
				msg("1");
			else
				msg("0");
			mask >>= 1;
		}
		SPACE();
	}
}

void DOT_D(void)  /* n -- */
{
	Byte b = (Byte)base_;

	DECIMAL();
	DOT();
	base_ = b;
}

void DOT_H(void)  /* n -- */
{
	Byte b = base_;

	HEX();
	_CELL_;
	_2STAR_;
	DOT_R();
	SPACE();
	base_ = b;
}

/* Tools */
// print as base digits right placed in field
void dotnb(Byte field, Byte digits, Cell n, Byte r)
{
	Byte b = base_;

	base_ = r;
	lit(field-digits), SPACES();
	lit(n), START_NUMBER_CONVERSION();
	while(digits--)	CONVERT_DIGIT();
	END_NUMBER_CONVERSION(), TYPE();
	base_ = b;
}


/* ==== Dump Utility ==== */
void dotn(Byte field, Byte digits, Cell n) // print as hex digits right placed in field
{
	dotnb(field, digits, n, 16);
}

void dotLongHex(Long l) // print a long value in hex
{
	SPACE();
	dotn(4, 4, (Cell)(l>>16));
	dotn(4, 4, (Cell)l);
}

void dotindex(Byte index) // print out heading indices
{
	Byte i;

	msg("\n           ");
	for (i=0; i<16; i++)
		dotn(3, 2, index++);
}

void dotaddr(Cell address) // print out address
{
	_CR(), dotn(8,sizeof(Cell)*2,address);
}

void dotfaddr(Cell address) // print out far address
{
	_CR(), dotn(7,4,address), msg("0");
}

void dotline(Byte *bytes) // print out one line of memory
{
	Byte i;
	
	msg(":  ");
	for (i=0; i<16; i++)
		dotn(3,2,bytes[i]);
	SPACE();
	for (i=0; i<16; i++)
	{
		Byte b = *bytes++;

		if ((b<' ') || (b>'~'))
			b = '.';
		lit(b), EMIT();
	}
}

// messages in flash to keep ram slimmer
Byte debug = TIMBRE_DEBUG;

void pmsg(PGM_P m) // message in program space
{
	Byte b;

	while ((b = pgm_read_byte(m++)) != 0)
		lit(b), EMIT();
}

void debugpmsg(PGM_P m)
{
	if (debug)
		pmsg(m);
}

void dump(void) // ram dump
{
	Byte n = (Byte)*_DROP_;
	Cell memory = *_DROP_;
	
	dotindex((Byte)memory);
	while (n--)
	{
		dotaddr(memory);
		dotline((Byte *)memory);
		memory += 16;
	}
}

// stack dump
/*
depth dup . ." xxxx" 10 min 0 max >r here r for !+ next cell - r> for @- over . next drop
*/

void DOT_S(void)  /* -- */
{
	Integer n;
	Cell d;
	
	DEPTH();
	n = (Integer)*sp;
	DOT();
	msg(" stack items: ");
	if (n > 10)
		n = 10;
	while (n > 0)
	{
		d = sp[--n];
		lit(d);
		DOT();
	}
}

// word list
/* Dictionaries to search
    wordnames wordbodies
    immediatenames immediatebodies
    constantnames constantbodies
*/
Byte search_names(Byte *name, PGM_P dictionary) // return name number or 0 if not found
{
	Byte index = 1;
	
	while(pgm_read_byte(dictionary))
	{
		if (strcmp_P(name, dictionary) == 0)
			return index;
		index++;
		dictionary += strlen_P(dictionary) + 1;
	}
	return 0;
}

void search_dictionaries(void) // look through dictionaries for word
{ // s -- a \ f
	Byte index, length = *(Byte *)(_TOP_);
	Byte name[31+1]; //[length+1];
	
	memcpy(&name[0], (Byte *)(_TOP_+1), length);
	name[length] = 0;
	if ((index = search_names(name, constantnames)) != 0)
	{
		_TOP_ = (Cell)&constantbodies[2*(index-1)]; // array of two pointers
		lit(0x80);
		return;
	}
	if ((index = search_names(name, wordnames)) != 0)
	{
		_TOP_ = (Cell)&wordbodies[index-1];
		lit(0x80);
		return;
	}
	if ((index = search_names(name, immediatenames)) != 0)
	{
		_TOP_ = (Cell)&immediatebodies[index-1];
		lit(0xC0);
		return;
	}
	lit(0);
}

void list_dictionaries(void) // list words in dictionaries
{
	PGM_P dictionary, *dictionaries[] = {constantnames, wordnames, immediatenames};
	Byte i, c;
	
	for (i=0; i< 3; i++)
	{
		dictionary = dictionaries[i];
		while((c = pgm_read_byte(dictionary++)) != 0)
		{
			do
			{
				lit(c), EMIT();
			} while ((c = pgm_read_byte(dictionary++)) != 0);
			lit(2), SPACES();
		}
	}
}

/*
: words  latest @ >r  yes  listq q?
   for if  listq p @ latest !  in @ >r  cr shell{ words }shell cr  r> in !
       else  no  then
       listq pop  listq stuff  next  drop  bl parse  r> latest ! ;
*/
void DOT_NAME(void)  /* lf -- */
{
	L_TO_NAME();
	COUNT();
	*sp&=31;     /* AND */
	TYPE();
	*--sp=2;
	SPACES();
}

void WORDS(void)  /* -- */
{
	QUESTION_CR();
	latest();
	for(;;)	/* BEGIN */
	{
		L_TO_LINK();
		QUESTION_DUP();
		if(!*sp++)break;     /* WHILE */
			DUP(), DOT_NAME();
	};       /* REPEAT */
	list_dictionaries();
}

void revcpy(Byte *dest, Byte *src, Byte length) // copy in reverse order
{
	Byte i=0;

	while(length) dest[i++] = src[--length];
}
