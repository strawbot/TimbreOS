#ifndef _COPYRUN_H_
#define _COPYRUN_H_

#include "memorymaps.h"
#include "versionNames.h"

// common header preceding images except for boot
#define RELEASE_DATE_LENGTH 32
#define APP_NAME_LENGTH 16

typedef struct {
	versionNumber mmb;
	unsigned long start;
	unsigned long dest;
	unsigned long size;
	unsigned long entry;
	unsigned long checksum;
	unsigned long headerSize;
	char releaseDate[RELEASE_DATE_LENGTH];		// address within image
	char appName[APP_NAME_LENGTH];			// ditto
	unsigned long headerChecksum;
}imageHead_t;

#define HEADER_SIZE sizeof(imageHead_t)

enum {CHECK_OK, ERR_OUTOFRANGE, ERR_CHECKSUM, ERR_COPY};

#endif

unsigned long copyRun(unsigned long image);
unsigned long checkImage(unsigned long image); // verify image
unsigned long checkHeader(unsigned long image); // verify image header
