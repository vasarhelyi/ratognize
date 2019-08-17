#ifndef HEADER_DATETIME
#define HEADER_DATETIME

#include <list>

typedef double timed_t;

/**
 * Parse date and time from a string, searching for all possible date formats.
 *
 * \param dstTime      the time value parsed
 * \param srcStr       the input string containing the date and time
 * \param bRelative    should we treat string as relative or absolute time?
 * \return negative value on error (how many has been parse),
 *         3 or 6 if date or date_time parsed successfully
 */
int ParseDateTime(timed_t * dstTime, char *srcStr, bool bRelative=false);

/**
 * Calculate the number of days since the last paint date.
 *
 * \param dayssincelastpaint   the calculated days are stored here if needed
 * \param inputvideostarttime  the time of the starting frame of the video
 * \param mPaintDates          databased storing all paint dates
 *
 * \return  true on success (which does not always mean that 
 *          dayssincelastpaint has been modified)
 */
bool SetDSLP(int* dayssincelastpaint, timed_t inputvideostarttime, 
		std::list<time_t>& mPaintDates);

#endif