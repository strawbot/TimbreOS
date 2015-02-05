// Version

#include "build_version.h"
#include "version.h"
#include <stdlib.h>
#include <string.h>
#include "valueTransfer.h"

#define BUILD_DATE __DATE__##" "##__TIME__
version_t version = {VERSION_ID, BUILD_NAME, BUILD_DATE};

char * getBuildDate(void)
{
	return &version.buildDate[0];
}

char * getBuildName(void)
{
	return &version.buildName[0];
}

unsigned long majorVersion(void) // use last two digits of year
{
	return extractMajorVersion(&version);
}

unsigned long minorVersion(void) // month 1-12
{
	return extractMinorVersion(&version);
}
	
unsigned long buildVersion(void) // return ((day-1)*24 + hour)*60 + minute
{
	return extractBuildVersion(&version);
}

// major, minor, build extraction	
unsigned long extractMajorVersion(version_t *version)
{
	char year[3];
	
	memcpy(year, &version->buildDate[9], 2);
	return strtoul(year, (char **)NULL, 10);
}

unsigned long extractMinorVersion(version_t *version)
{
	if (0 == memcmp(&version->buildDate[0], "Jan", 3)) return 1;
	if (0 == memcmp(&version->buildDate[0], "Mar", 3)) return 2;
	if (0 == memcmp(&version->buildDate[0], "Mar", 3)) return 3;
	if (0 == memcmp(&version->buildDate[0], "Apr", 3)) return 4;
	if (0 == memcmp(&version->buildDate[0], "May", 3)) return 5;
	if (0 == memcmp(&version->buildDate[0], "Jun", 3)) return 6;
	if (0 == memcmp(&version->buildDate[0], "Jul", 3)) return 7;
	if (0 == memcmp(&version->buildDate[0], "Aug", 3)) return 8;
	if (0 == memcmp(&version->buildDate[0], "Sep", 3)) return 9;
	if (0 == memcmp(&version->buildDate[0], "Oct", 3)) return 10;
	if (0 == memcmp(&version->buildDate[0], "Nov", 3)) return 11;
	if (0 == memcmp(&version->buildDate[0], "Dec", 3)) return 12;
	return 0;
}

unsigned long extractBuildVersion(version_t *version)
{
	char day[3] = {0, 0, 0};
	char hour[3] = {0, 0, 0};
	char minute[3] = {0, 0, 0};
	unsigned long build;
	
	memcpy(&day[0], &version->buildDate[4], 2);
	memcpy(&hour[0], &version->buildDate[12], 2);
	memcpy(&minute[0], &version->buildDate[15], 2);

	build = strtoul(day, (char **)NULL, 10);
	build = (build - 1)*24 + strtoul(hour, (char **)NULL, 10);
	return build * 60 + strtoul(minute, (char **)NULL, 10);
}

unsigned long extractVersion(unsigned long address) // address might not be long aligned
{
	version_t *version = (version_t *)address;
	unsigned long mmb = 0;
	
	if (bytesToLong((Byte *)&version->headerTag) == VERSION_ID)
	{
		mmb = extractMajorVersion(version);
		mmb = (mmb<<8) + extractMinorVersion(version);
		mmb = (mmb<<16) + extractBuildVersion(version);
	}
	return mmb;
}

// Setting prompt using Build name
#define MAX_PROMPT 20
#define PROMPT_COLON ": "

void setPrompt(char * );

void setBuildPrompt(void);
void setBuildPrompt(void)
{
	char prompt[MAX_PROMPT+1] = {0};

	strncat(prompt, getBuildName(), MAX_PROMPT - sizeof(PROMPT_COLON));
	strcat(prompt, PROMPT_COLON);
	setPrompt(prompt);
}