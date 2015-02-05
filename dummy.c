/* Dummy definitions for Timbre library  Rob Chapman  Jun 8, 2011
  modify as needed.
  
Need to define: - all of type: void (void)

getInput - must fill kq
sendeq - must empty eq

can be dummied:
setup_io - take care of any io initialization

led on and offs:
greenOff - power
greenOn
yellowOff - input
yellowOn

initApp - if app needs inits
initTimeStamp - for time stamp initialization
*/
#include <stdio.h>
#include "bktypes.h"
#include "byteq.h"

// IO setup and in/out
Byte lastkey = 0;
extern Byte eq[];
void keyin(Byte c);

// prototypes
void setup_io(void);
void restore_io(void);
void getInput(void);
void sendeq(void);
void txUartString(Byte *s);
void greenOff(void);
void greenOn(void);
void yellowOff(void);
void yellowOn(void);
void initApp(void);

void setup_io(void)  // take care of any io initialization
{ }

void restore_io(void) {}

void getInput(void)  // must fill kq
{
    int c = getchar();

	if (c != -1)
	{
		if (c == 0xA)
			c = 0xD;
		keyin((Byte)c);
	}
}

void sendeq(void)  // must empty eq
{
Byte c;
while (qbq(eq))
    {
       c = pullbq(eq);
       if (c != 0xD)
		putchar(c);
    }
}

void txUartString(Byte *s)
{
    puts((char *)s);
}

// Leds
void greenOff(void)  // power
{ }

void greenOn(void)
{ }

void yellowOff(void)  // input
{ }

void yellowOn(void)
{ }

// App inits
void autoEchoOn(void);

void initApp(void)  // for time stamp initialization
{
  autoEchoOn();
}
