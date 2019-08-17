#ifndef HEADER_COLOR
#define HEADER_COLOR

#include <list>

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include <time.h>

#include "constants.h"
#include "light.h"

// color interpolation types
typedef enum {
    COLOR_INTERPOLATE = 0,
    COLOR_FIT_LINEAR = 1,
    COLOR_INTERPOLATE_DATE = 2,
} color_interpolation_t;

//! A structure holding all information related to the
// extraction of one color in HSV space.
typedef struct {
    CvScalar mColorHSV;         //!< (configuration) Hue(0-180)-Saturation(0-255)-Value(0-255)
    CvScalar mRangeHSV;         //!< (configuration) +- range for the Hue-Saturation-Value channel
} tColor;

// Class used for current color definition and in
// std::list for color database when paint is fading
class cColorSet {
  public:
    int day;
    time_t datetime;
    tColor mColor[MAXMBASE];    // [color0,color1,...]
	tColor mBGColor;            // background color
    bool operator <(const cColorSet & rhs) {
        return day < rhs.day;
    }                           // used for sorting
    //! Constructor.
	cColorSet() {
        Reset();
    }
    //! Destructor.
    ~cColorSet() {
    }
    // reset
    void Reset(bool bResetBGColor=true) {
        for (int i = 0; i < MAXMBASE; i++) {
            mColor[i].mColorHSV = cvScalar(0, 0, 0);
            mColor[i].mRangeHSV = cvScalar(0, 0, 0);
        }
		if (bResetBGColor) {
			mBGColor.mColorHSV = cvScalar(0, 0, 0);
			mBGColor.mRangeHSV = cvScalar(0, 0, 0);
		}
        day = 0;
        datetime = 0;
    }
};

// A simple class holding all information related to one color
class cColor {
  public:
    tColor mColor;              // the actually used color definition
    bool mUse;                  //!< (configuration) Use this color? (to check individual channel outputs and treat mBase<6 cases)
    char name[10];              // short name of the color (max 9 chars)
    int mNumBlobsFound;         // how many bins have been found of this color at a given step?
    //! Constructor.
    cColor() {
        Reset();
    }
    //! Destructor.
	~cColor() {
    }
    // reset
    void Reset() {
        mColor.mColorHSV = cvScalar(0, 0, 0);
        mColor.mRangeHSV = cvScalar(0, 0, 0);
        mNumBlobsFound = 0;
        mUse = false;
        name[0] = 0;
    }
};

// callback function for sorting by date instead of daysincelastpaint
bool compareCColorSetsByDate(const cColorSet & a, const cColorSet & b);

/**
 * This function should be called dinamically when a switch in light condition
 * is detected to choose/update the corresponding color definition.
 *
 * \param light   the light type that is used (DAYLIGHT, NIGHTLIGHT, etc.)
 * \param method  color interpolation method
 * \param dslp    days since last paint value
 * \param inputvideostarttime  the starting time of the input video
 * \param mColorDataBase  the entire color database consisting of all day and
 *                        night color definitions in an array of lists, indexed
 *                        by the two (DAY and NIGHT) light definitions
 * \param mColor   the destination list where the defined colors will be stored
 * \param mBGColor the destination where the background color will be stored
 *
 *
 * \return true on success, false otherwise
 */
bool SetHSVDetectionParams(lighttype_t light, color_interpolation_t method,
        int dslp, double inputvideostarttime,
        std::list<cColorSet>* mColorDataBase, cColor* mColor, tColor* mBGColor);

#endif