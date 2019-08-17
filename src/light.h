#ifndef HEADER_LIGHT
#define HEADER_LIGHT

// light types
typedef enum {
    UNINITIALIZEDLIGHT = -1,    // night setting not initialized
    DAYLIGHT = 0,               // daylight
    NIGHTLIGHT = 1,             // nightlight
    EXTRALIGHT = 2,             // daylight and possibly extra light is on
    STRANGELIGHT = 3            // nightlight but LED blob is found (???)
} lighttype_t;

// name strings for light types
extern const char *lighttypename[4];

#endif
