#include "bktypes.h"

#ifndef _VERSION_H_
#define _VERSION_H_

#define VERSION_ID 0xB11DDA7E // BILDDATE

typedef struct {
	unsigned long headerTag;
	char buildName[16];
	char buildDate[32];
} version_t;

/*
	__DATE__  "Feb 12 1996"
	__TIME__  "23:59:01"
	build date: "Feb 12 1996 23:59:01"
						   1111111111
	             01234567890123456789
*/

char * getBuildDate(void);
char * getBuildName(void);
unsigned long majorVersion(void);
unsigned long minorVersion(void);
unsigned long buildVersion(void);

unsigned long extractMajorVersion(version_t *version);
unsigned long extractMinorVersion(version_t *version);
unsigned long extractBuildVersion(version_t *version);
unsigned long extractVersion(unsigned long address);

void setBuildPrompt(void);
void showVersion(void);

#endif