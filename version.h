#include "bktypes.h"

#ifndef _VERSION_H_
#define _VERSION_H_

#define VERSION_ID 0xB11DDA7E // BILDDATE

typedef struct {
	Long headerTag;
	char buildName[16];
	char buildDate[32];
} version_t;

typedef struct {
	union {
		Long version;
		struct {
			Byte major;
			Byte minor;
			Short build;
		};
	};
} mmb_t;

/*
	__DATE__  "Feb 12 1996"
	__TIME__  "23:59:01"
	build date: "Feb 12 1996 23:59:01"
						   1111111111
	             01234567890123456789
*/

char * getBuildDate(void);
char * getBuildName(void);
Long majorVersion(void);
Long minorVersion(void);
Long buildVersion(void);

Long extractMajorVersion(version_t *version);
Long extractMinorVersion(version_t *version);
Long extractBuildVersion(version_t *version);
Long extractVersion(Long address);
Long getVersion(void);

void setBuildPrompt(void);
void showVersion(void);

#endif