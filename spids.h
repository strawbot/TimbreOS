// transfer spids
enum {
	TRANSFER_REQUEST,	// size in bytes (32 bit), name (count prefixed), type (byte)
	TRANSFER_REPLY,		// status (byte)
	TRANSFER_DATA,		// data
	TRANSFER_RESULT,		// status
	TRANSFER_DONE,
	// results
	REQUEST_OK, REQUEST_TOOBIG, TRANSFER_BUSY, // from request
	TRANSFER_OK, CHECK_ERROR, TRANSFER_INCOMPLETE, UNSUPORTED_TYPE, // from transfer
	// types
	UNKNOWN, JAM_PLAYER, JBC_PLAYER, TEXT_TRANSFER,
	TRANSFER_FILE, // file name, type > reply with size and checksum
	TRANSFER_CHUNK, // address, size > address (32), size (32), data
	FILE_UNAVAILABLE, // either the file name is wrong or resource unavailble
	REQUEST_FAIL // handles unknown request issues
};
