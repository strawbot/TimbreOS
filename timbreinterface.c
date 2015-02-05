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

#include <mqx.h>
#include <fio.h>
#include <shell.h>

typedef unsigned char Byte;
Byte qbq(Byte *);
Byte pullbq(Byte *);
void keyin(Byte);
extern Byte eq[];
int exit_timbre;
void txUartString(char *);

#define ESCAPE 0x1B

// prototypes
void timbre(void);
void _sched_yield(void);
int_32 Shell_timbre( int_32 argc, char_ptr argv[] );
void setup_io(void);
void getInput(void);
void sendeq(void);
void greenOff(void);
void greenOn(void);
void yellowOff(void);
void yellowOn(void);
void initApp(void);
void initTimeStamp(void);

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    :    Shell_timbre
* Returned Value   :    int_32 error code
* Comments         :    command line access to memory. 
*
*END*---------------------------------------------------------------------*/

int_32 Shell_timbre( int_32 argc, char_ptr argv[] )
{
   boolean                       print_usage, shorthelp = FALSE;
   int_32                        return_code = SHELL_EXIT_SUCCESS;

   print_usage = Shell_check_help_request(argc, argv, &shorthelp );

   if (!print_usage)  
   {
	   exit_timbre = 0;
	   while(!exit_timbre)
	   {
	       _sched_yield();
	       timbre();
	   }
	   printf("done.\n");
   }
   else  
   {
#ifdef __mcf5227_h__ // different help display on different cpus!
	   if (shorthelp)
		   printf("\n---> %s\n", argv[0]);
	   else
		   printf("\nUsage: %s", argv[0]);
#else
	   if (shorthelp)
		   printf("%s\n", argv[0]);
	   else
		   printf("Usage: %s\n", argv[0]);
#endif
   }
   return return_code;
}

// IO setup and in/out
void setup_io(void)  // take care of any io initialization
{ }

void getInput(void)  // must fill kq
{
	if (status())
	{
		Byte x = (Byte)getchar();
		
		if (x == ESCAPE)
			exit_timbre = 1;
		else
		{
			if (x == 0xa)
				x = 0xd;
			keyin(x);
		}
	}
}

void sendeq(void)  // must empty eq
{
	while (qbq(eq))
		putchar(pullbq(eq));
}

void txUartString(char *string)
{
	while (*string)
		putchar(*string++);	
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
void initApp(void)  // for time stamp initialization
{ }

void initTimeStamp(void)  // for time stamp initialization
{ }

