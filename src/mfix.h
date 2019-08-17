#ifndef HEADER_MFIX
#define HEADER_MFIX

// mFix type flags - should be consistent with trajognize codes
typedef enum {
    MFIX_FULLFOUND = 1,
    MFIX_SHARESID = 2,
    MFIX_SHARESBLOB = 4,
    MFIX_PARTLYFOUND_FROM_TDIST = 8,
    MFIX_DELETED = 16,
    MFIX_CHOSEN = 32,
    MFIX_FULLNOCLUSTER = 64,
    MFIX_CHANGEDID = 128,
    MFIX_VIRTUAL = 256,
    MFIX_DEBUG = 512
} mfix_t;

/* old codes
typedef enum {
	MFIX_FULLFOUND = 1,
	MFIX_SHARESID = 2,
	MFIX_SHARESBLOB = 4,
	MFIX_PARTLYFOUND = 8,
	MFIX_PREDICTED = 16,
	MFIX_LOST = 32,
	MFIX_MDFOUND = 64,
	MFIX_BWFOUND = 128,
	MFIX_DELETED = 256
} mfix_t;
*/

#endif