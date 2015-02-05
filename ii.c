/* Inner interpreters  Rob Chapman  Jul 21, 1992 */
// Copyright TimbreWorks Engineering 1990-2005

#include "botkernl.h"	/* stacks and stuff */

/* : vii  ( -- a )  ip @ @  CELL + ;  */
void VII()	/* -- ii */
{
	*--sp=(Cell)(tick+1);
}

/* : cii  ( -- n )  vii @ ; */
void CII()	/* -- ii */
{
	*--sp=(Cell)*(tick+1);
}

/*
  : EXECUTE  ( tick -- )  DUP tick !  @ JSR ;
  : ITC  ( -- )  BEGIN  ip @  @+  ip !  ?DUP  WHILE  EXECUTE  REPEAT ;
  : :II  ( -- )  ip @ >R  tick @ CELL + ip !  ITC  R> ip ! ;
*/
void COLON_II()  /* -- ii */
{
	thread *sip = ip;

	ip = (thread *)(tick+1);
	while((tick=*ip++) != 0)
		(**tick)();	  /* EXECUTE */
	ip = sip;
}
