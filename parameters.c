// Setting parameters for IO  Robert Chapman  Aug 31, 2012

#include "bktypes.h"
#include "parameters.h"
#include "printers.h"

void setParam(Long tag, Long value);
Long getParam(Long tag);

// parameters

void setParam(Long tag, Long value)
{
	switch(tag)
	{
		case AUTOBOOT_PARAM:
			setAutoBoot(value);
			break;
		default:
			print("unknown parameter: ");
			printDec(tag);
			print(" with value: ");
			printDec(value);
	}
}

Long getParam(Long tag)
{
	switch(tag)
	{
		case AUTOBOOT_PARAM:
			return getAutoBoot();
		default:
			print("unknown parameter: ");
			printDec(tag);
			return 0;
	}
}
