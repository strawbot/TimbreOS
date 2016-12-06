#include "bktypes.h"

// Link structure
typedef struct serialLink_t{ // Link information
	// common
	void * port;								// data structure for port for link

	// Receiver
	void (* linkIn)(struct serialLink_t *);		// receive the byte

	// Transmitter
	Byte * txq;									// point to queue of outgoing bytes
	bool (*linkEmpty)(struct serialLink_t *);	// nothing left to send?
	void (*linkOut)(struct serialLink_t *);		// put the byte plus any upper bits

} serialLink_t;
