#include "machines.h"
#include "botkernl.h"

void initApp(void);
void PARSE(void);
void HERE(void);

char * getArg(void)
{
	char * arg;

	lit(0), PARSE(), HERE();
	arg = (char *)ret;
	return ++arg;
}

void cli(void)
{
	initApp();
    while(true)
      runMachines();
}
