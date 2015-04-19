#ifndef _COPYRUN_H_
#define _COPYRUN_H_

#include "version.h"

// common header preceding images except for boot
#define RELEASE_DATE_LENGTH 32
#define APP_NAME_LENGTH 16

typedef struct {
	Long mmb;
	Long start;
	Long dest;
	Long size;
	Long entry;
	Long checksum;
	Long headerSize;
	char releaseDate[RELEASE_DATE_LENGTH];		// address within image
	char appName[APP_NAME_LENGTH];			// ditto
	Long headerChecksum;
}imageHead_t;

#define HEADER_SIZE sizeof(imageHead_t)

enum {CHECK_OK, ERR_OUTOFRANGE, ERR_CHECKSUM, ERR_COPY};

#endif

Long copyRun(Long image);
Long checkImage(Long image); // verify image
Long checkHeader(Long image); // verify image header
