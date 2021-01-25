#define _USE_MATH_DEFINES
#include <cmath>

/////////////////////////////////////////////////
// include OpenCV header files
#include <opencv2/opencv.hpp>

#include "cvutils.h"
#include "cage.h"
#include "light.h"

////////////////////////////////////////////////////////////////////////////////
// should be called (before ROI is defined) to detect RED LED state
// avg intensity sets day/night light, but red LED detection can change it to EXTRA/STRANGE
// param: HSV image converted from smoothinputimage
bool ReadDayNightLED(cv::Mat &hsv, cv::Mat &inputimage, std::ofstream& ofslog,
		cCS* cs, std::list<cColorSet>* mColorDataBase,  cColor* mColor,
		tColor* mBGColor, lighttype_t* mLight,
		timed_t inputvideostarttime, int currentframe) {
    static const int minLEDblobsize = 50; // it used to be 100 but 50 is better according to sample_trial_run measurements
    static lighttype_t lastLight = UNINITIALIZEDLIGHT;
    const int imsize = 200;     // 200 is OK to fit all cage movements without problems
    int isdaylight = 0;         // quorum response counter for RGB channels
	cv::Mat hsvroi;

    // calculate average intensity of image. This hopefully clearly separates DAY and NIGHT light conditions.
    // threshold values are determined from avg intensity histograms of 70 random videos:
    // Note: these thresholds were calculated on old imageROI (250 0 1344 1080),
    // new one (125 0 1600 1080) results in slightly different values of averages (+-5),
    // but the same thresholds can be used without problems (based on 4000 videos from 2011-05-04 to 2011-09-27).
    // R - 111
    // G - 100
    // B - 80
    cv::Scalar avgBGR = cv::mean(inputimage);
    // check red
    if (avgBGR.val[2] > 111)
        isdaylight++;
    // check green
    if (avgBGR.val[1] > 100)
        isdaylight++;
    // check blue
    if (avgBGR.val[0] > 80)
        isdaylight++;

    // set smaller image ROI to speed up calculation
    cv::Rect rect(
			std::max(cs->mLEDPos.x - imsize / 2, 0),
            std::max(cs->mLEDPos.y - imsize / 2, 0),
            imsize, imsize);
    hsvroi = hsv(rect);
    //cv::imshow("debug", hsv);
    static cv::Mat filterimage;
    filterimage = cvCreateImageOnce(filterimage, hsvroi.size(), 8, 1, false);  // no need to zero it
    // find hsv blob
    cvFilterHSV(filterimage, hsvroi, cs->mLEDColor.mColorHSV,
            cs->mLEDColor.mRangeHSV);
    cv::dilate(filterimage, filterimage, cv::Mat(), cv::Point(-1, -1), 2);
    cv::erode(filterimage, filterimage, cv::Mat(), cv::Point(-1, -1), 2);
    if (cs->bShowDebugVideo)
        cv::imshow("LED", filterimage);

    // Init blob extraction
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Moments moments;
    unsigned int j = 0;
    double maxmomentsize = 0;

	// find blobs
    cv::findContours(filterimage, contours, hierarchy, cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_NONE, cv::Point(0, 0));
    // Iterate over first blobs (allow for more than final number, but not infinitely)
    while (j < contours.size()) {
        // Compute the moments
        moments = cv::moments(contours[j]);
        if (moments.m00 > maxmomentsize)
            maxmomentsize = moments.m00;
    }

    // if no LED is used, default is NIGHTLIGHT
    if (!cs->bLED) {
        *mLight = NIGHTLIGHT;
    }
    // if big enough blob found, set to DAYLIGHT (or STRANGELIGHT)
    else if (maxmomentsize >= minLEDblobsize) {
        // majority votes for daylight
        if (isdaylight > 1)
            *mLight = DAYLIGHT;
        // avg votes for nightlight but a LED blob is found. How come?
        else
            *mLight = STRANGELIGHT;
    }
    // no LED blob found, determine if night or extra
    else {
        // everyone votes for nightlight
        if (isdaylight < 2)
            *mLight = NIGHTLIGHT;
        // avg votes for daylight but no LED blob found --> possibly EXTRA light is on.
        else
            *mLight = EXTRALIGHT;
    }

    // change settings and write change to log file
    if (*mLight != lastLight) {
        if (!SetHSVDetectionParams(*mLight, cs->colorselectionmethod,
				cs->dayssincelastpaint, inputvideostarttime,
				mColorDataBase, mColor, mBGColor)) {
            return false;
        }
        ofslog << currentframe << "\tLED\t" << lighttypename[*mLight] << std::endl;
        lastLight = *mLight;
    }
    // write average intensity (RGB), number of votes to daylight and max LEDblob size found
    ofslog << currentframe << "\tAVG\t" << avgBGR.val[2] << "\t" << avgBGR.
            val[1] << "\t" << avgBGR.
            val[0] << "\t" << isdaylight << "\t" << maxmomentsize << std::endl;

    // return without error
    return true;
}
