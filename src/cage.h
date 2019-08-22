#ifndef HEADER_CAGE
#define HEADER_CAGE

#include "ini.h"

/**
 * Automated LED detection designed specifically for the ELTE 2011 experiment, where
 * a red led indicated the light setting (DAYLIGHT or NIGHTLIGHT).
 *
 * \param hsvimage        the input image in hsv color space
 * \param inputimage      the original image that is modified to show the LED
 * \param ofslog          the log file where the LED params will be stored
 * \param cs              control settings structure
 * \param mColorDataBase  the color database that stores all color defs
 * \param mColor          the list of colors used currently
 * \param mBGColor        the background color used currently
 * \param mLight          the output light setting based on the LED detection
 * \param inputvideostarttime  the starting time of the input video
 * \param currentframe    the current video frame used
 *
 * Function writes into the log file and sets mLight param with the
 * detected light setting.
 */
bool ReadDayNightLED(cv::Mat &hsv, cv::Mat &inputimage, std::ofstream& ofslog,
		cCS* cs, std::list<cColorSet>* mColorDataBase,  cColor* mColor,
		tColor* mBGColor, lighttype_t* mLight,
		timed_t inputvideostarttime, int currentframe);


#endif
