#ifndef HEADER_RATOGNIZE
#define HEADER_RATOGNIZE

/////////////////////////////////////////////////
// include OpenCV header files
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

/////////////////////////////////////////////////
// include standard C++ libraries
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <iterator>
#include <time.h>

/////////////////////////////////////////////////
// include from self project
#include "ini.h"
#include "light.h"
#include "mfix.h"

#define RATOGNIZE_H__VERSION "$Id: ratognize.h 9957 2018-07-11 13:55:27Z vasarhelyi $"

#define CV_WARN(message) fprintf(stderr, "warning: %s (%s:%d)\n", message, __FILE__, __LINE__)

clock_t measure_duration_start, measure_duration_stop;
#ifdef ON_LINUX
#define MEASURE_DURATION(NAME) \
	measure_duration_start = clock(); \
	 NAME  ; \
	measure_duration_stop = clock(); \
	if (cs.bCout) cout << "time:  NAME : " << (((measure_duration_stop - measure_duration_start) * 1000) / CLOCKS_PER_SEC) << " ms" << endl;
#else
#define MEASURE_DURATION(NAME) \
	measure_duration_start = clock(); \
	## NAME ##  ; \
	measure_duration_stop = clock(); \
	if (cs.bCout) cout << "time: " #NAME ": " << (((measure_duration_stop - measure_duration_start) * 1000) / CLOCKS_PER_SEC) << " ms" << endl;

#endif
// To disable benchmarking, uncomment following line: 
//#define MEASURE_DURATION(NAME) NAME;

/* fix "‘INT64_C’ was not declared in this scope" errors,
   see: http://vskrishnan.wordpress.com/2007/07/31/ffmpeg-int64_c-problem-and-workaround/ */
//#ifdef ON_LINUX
//#define INT64_C
//#define __STDC_CONSTANT_MACROS
//#include <stdint.h>
//#endif

/////////////////////////////////////////////////
// variables

// from ini file
cCS cs;                         //!< all control states read from the ini file
std::list < cColorSet > mColorDataBase[2]; // the full color database list [day/night]
cColor mColor[MAXMBASE];        //!< actual colors to detect - parsed dynamically from list/interpolation
tColor mBGColor;                //!< actual background color definition

// from paintdates file
std::list < time_t > mPaintDates;       // seconds since 1970 1 January

lighttype_t mLight;             // DAYLIGHT, NIGHTLIGHT or EXTRALIGHT (as part of daylight)

// variables for blob detection
tBarcode mBarcodes;             // barcodes loaded from trajognize output
tBlob mBlobParticles;           //!< The list of detected colored-particles.
tBlob mMDParticles;             // list of motion-detected blobs
tBlob mRatParticles;            // list of rat blobs
IplImage *movingAverage;        // used by the motion detection filter

// image, video and text output parameters
CvSize framesize;
CvSize framesizeROI;
int framecount;
double fps;
int currentframe;
IplImage *inputimage;           // BGR original image read from the video
IplImage *smoothinputimage;     // BGR original image read from the video and smoothed
IplImage *HSVimage;             // HSV image to work on with all image prcessing tools
IplImage *maskimage;            // binary mask image containing only enlarged rat blobs
CvCapture *inputvideo;
timed_t inputvideostarttime;    // like time_t but increased with fraction of a second

// stream and string variables
std::ifstream ifsbarcode;
std::ifstream ifsdat;
std::ifstream ifslog;
std::ofstream ofsdat;
std::ofstream ofslog;
std::string args;

/////////////////////////////////////////////////
// functions

int OnInit(int argc, char *argv[]); // called at initialization once, returns error code, which is positive if release is needed in consecutive OnExit()
bool initializeVideo(char *filename); // called by OnInit() once
bool readVideoUntilFirstGoodFrame(); // called by OnInit() once
bool OnStep();                  // called on each frame
bool ReadNextFrame();           // called by OnStep(), reads next frame to input image
void GenerateOutput();          // called by OnStep(), generate video, image, text, etc.
void OnExit(bool bReleaseVars = true);  // called once to release all allocated memory

#endif
