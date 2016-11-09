// Default SFP Parameters  Robert Chapman III  Feb 21, 2015

// make a copy in application directory
// change parameters for application
// make sure application copy is first in the include list
// can include simple numeric parameters in pids.txt; then will be available in python

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#define NUM_LINKS 2				// number of links in this node

#include <libarm.h>

#define intDisable()  libarm_disable_irq_fiq() 	// Disable Interrupts
#define intEnable()  libarm_enable_irq_fiq() 	// Enable Interrupts

#endif