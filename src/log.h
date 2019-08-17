#ifndef HEADER_LOG
#define HEADER_LOG

#include <stdio.h>
#include <stdlib.h>

#define LOG_ERROR(fmt, ...) \
	fprintf(stdout, "[ERROR] %s(), %s:%d: " fmt "\n", __FUNCTION__, __FILE__, \
            __LINE__, ## __VA_ARGS__); \
	fflush(stdout);

#endif
