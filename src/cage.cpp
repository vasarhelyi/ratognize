#define _USE_MATH_DEFINES
#include <cmath>

/////////////////////////////////////////////////
// include OpenCV header files
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include "cvutils.h"
#include "cage.h"
#include "light.h"

////////////////////////////////////////////////////////////////////////////////
// should be called (before ROI is defined) to detect RED LED state
// avg intensity sets day/night light, but red LED detection can change it to EXTRA/STRANGE
// param: HSV image converted from smoothinputimage
bool ReadDayNightLED(IplImage * hsv, IplImage * inputimage, std::ofstream& ofslog,
		cCS* cs, std::list<cColorSet>* mColorDataBase,  cColor* mColor, 
		tColor* mBGColor, lighttype_t* mLight, 
		timed_t inputvideostarttime, int currentframe) {
    static const int minLEDblobsize = 50; // it used to be 100 but 50 is better according to sample_trial_run measurements
    static lighttype_t lastLight = UNINITIALIZEDLIGHT;
    const int imsize = 200;     // 200 is OK to fit all cage movements without problems
    int isdaylight = 0;         // quorum response counter for RGB channels

    // calculate average intensity of image. This hopefully clearly separates DAY and NIGHT light conditions.
    // threshold values are determined from avg intensity histograms of 70 random videos:
    // Note: these thresholds were calculated on old imageROI (250 0 1344 1080),
    // new one (125 0 1600 1080) results in slightly different values of averages (+-5),
    // but the same thresholds can be used without problems (based on 4000 videos from 2011-05-04 to 2011-09-27).
    // R - 111
    // G - 100
    // B - 80
    CvScalar avgBGR = cvAvg(inputimage);
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
    CvRect rect =
            cvRect(std::max(cs->mLEDPos.x -
                    (inputimage->roi ? inputimage->roi->xOffset : 0) -
                    imsize / 2, 0),
            std::max(cs->mLEDPos.y -
                    (inputimage->roi ? inputimage->roi->yOffset : 0) -
                    imsize / 2, 0),
            imsize, imsize);
    cvSetImageROI(hsv, rect);
    //cvShowImage("debug", hsv);
    static IplImage *filterimage = NULL;
    filterimage = cvCreateImageOnce(filterimage, cvGetSize(hsv), 8, 1, false);  // no need to zero it
    // find hsv blob
    cvFilterHSV2(filterimage, hsv, &cs->mLEDColor.mColorHSV,
            &cs->mLEDColor.mRangeHSV);
    cvDilate(filterimage, filterimage, NULL, 2);
    cvErode(filterimage, filterimage, NULL, 2);
    if (cs->bShowDebugVideo)
        cvShowImage("LED", filterimage);
    // Init blob extraction
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvContourScanner blobs =
            cvStartFindContours(filterimage, storage, sizeof(CvContour),
            CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    // init variables
    CvSeq *contour;
    CvMoments moments;
    double maxmomentsize = 0;

    // Iterate over first blobs (allow for more than final number, but not infinitely)
    while (1) {
        // Get next contour (if one exists)
        contour = cvFindNextContour(blobs);
        if (!contour)
            break;

        // Compute the moments
        cvMoments(contour, &moments);
        if (moments.m00 > maxmomentsize)
            maxmomentsize = moments.m00;
        // Release the contour
        cvRelease((void **) &contour);
    }
    // release memory and reset settings
    cvEndFindContours(&blobs);
    cvReleaseMemStorage(&storage);
    //cvReleaseImage(&filterimage);
    cvResetImageROI(hsv);

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

