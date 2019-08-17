#include <stdio.h>
#include <time.h>

#include "datetime.h"
#include "log.h"

int ParseDateTime(timed_t * dstTime, char *srcStr, bool bRelative) {
    tm temptime;
    int i = 0;
    // second with possible fraction is stored in *dstTime
    temptime.tm_sec = 0;

    // check project maze group line format (date only)
    if (i != 6) {
        i = sscanf(srcStr, "group%*d_%4d%2d%2d_M%*d_target%*d.ts",
                &temptime.tm_year, &temptime.tm_mon, &temptime.tm_mday);
        if (i == 3) {
            temptime.tm_hour = 0;
            temptime.tm_min = 0;
            *dstTime = 0;
            i = 6;
        }
    }
    // check project maze single line format (date only)
    if (i != 6) {
        i = sscanf(srcStr, "single%*d_%4d%2d%2d_*M_target%*d.ts",
                &temptime.tm_year, &temptime.tm_mon, &temptime.tm_mday);
        if (i == 3) {
            temptime.tm_hour = 0;
            temptime.tm_min = 0;
            *dstTime = 0;
            i = 6;
        }
    }
    // check project maze learn line format (date only)
    if (i != 6) {
        i = sscanf(srcStr, "learn%*d_%4d%2d%2d_*M*_target%*d.ts",
                &temptime.tm_year, &temptime.tm_mon, &temptime.tm_mday);
        if (i == 3) {
            temptime.tm_hour = 0;
            temptime.tm_min = 0;
            *dstTime = 0;
            i = 6;
        }
    }
    // check standard line format (date only, or date and time)
    // already store seconds with/without fraction
    if (i != 6) {
        i = sscanf(srcStr, "%4d-%2d-%2d_%2d-%2d-%lf", &temptime.tm_year,
                &temptime.tm_mon, &temptime.tm_mday, &temptime.tm_hour,
                &temptime.tm_min, dstTime);
        if (i == 3) {
            temptime.tm_hour = 0;
            temptime.tm_min = 0;
            *dstTime = 0;
        } else if (i != 6) {
            return -i;
        }
    }

    // convert time to tm time format
    // http://www.cplusplus.com/reference/clibrary/ctime/mktime/

    // check if we need a relative time
    if (bRelative) {
        *dstTime += temptime.tm_min * 60 +      // 1 min = 60 sec
                temptime.tm_hour * 3600 +       // 1 hour = 60 min
                temptime.tm_mday * 86400 +      // 1 day = 24 hour
                temptime.tm_mon * 2592000 +     // 1 month = 30 day
                temptime.tm_year * 31536000;    // 1 year = 365 day
    // or an absolute one
    } else {

        temptime.tm_year -= 1900;       // year is counted from 1900
        temptime.tm_mon -= 1;   // month is counted from 0
        temptime.tm_isdst = -1;
        // store only seconds since 00:00 hours, Jan 1, 1970 UTC.
        // http://www.cplusplus.com/reference/clibrary/ctime/time_t/
        *dstTime += (int) mktime(&temptime);
    }

    // return number of date elements parsed
    return i;
}

////////////////////////////////////////////////////////////////////////////////
// this function tries to set the dayssincelastpaint parameter by
// comparing the inputvideo file name and the mPaintDates database
// WARNING: there is no much error check on input data!
// the function also sets the hipervideoframestart and hipervideoframeend parameters
bool SetDSLP(int* dayssincelastpaint, timed_t inputvideostarttime, std::list<time_t>& mPaintDates) {
	// checks whether we have absolute date at all and return silently with 0
    if (inputvideostarttime == 0) {
        return true;
	}

    // calculate dayssincelastpaint
    // databases are (and must be) pre-sorted according to increasing day value
    std::list<time_t>::iterator it = mPaintDates.begin();
    while (it != mPaintDates.end() && (*it) < inputvideostarttime) {
        it++;
    }
    // error, date is earlier than first paint date
    if (it == mPaintDates.begin()) {
        LOG_ERROR("First date in paint date file is already after the date found in the inputvideo file name.");
        return false;
    }
    // no error, set variable
    it--;
    *dayssincelastpaint = (int) (inputvideostarttime - (*it)) / 86400;
	// return without error
    return true;
}
