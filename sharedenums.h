// Shared enums
#ifndef _SHARED_ENUMS_
#define _SHARED_ENUMS_

// Nexus DHT Type defines
#ifndef __cplusplus
    typedef
#endif
enum SYSMGR_DHT_TYPE
{
	SYSMGR_DHT_TYPE_NONE = 0,
	SYSMGR_DHT_TYPE_PIC,
	SYSMGR_DHT_TYPE_EXT,
	SYSMGR_DHT_TYPE_FIC,
	SYSMGR_DHT_TYPE_ENDURANT,
	SYSMGR_DHT_TYPE_MAXNUM
#ifndef __cplusplus
}SYSMGR_DHT_TYPE;
#else
};
#endif

#ifndef __cplusplus
    typedef
#endif
enum SYSMGR_HOA_MODE
{
	SYSMGR_HOA_UNKNOWN = 0,						
	SYSMGR_HOA_MANUALOFF,						
	SYSMGR_HOA_HAND,						
	SYSMGR_HOA_AUTO,						

	SYSMGR_HOA_MAXNUM
#ifndef __cplusplus
}SYSMGR_HOA_MODE;
#else
};
#endif

#endif