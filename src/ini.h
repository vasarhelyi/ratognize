#ifndef HEADER_INI
#define HEADER_INI

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>

#include "constants.h"
#include "color.h"
#include "datetime.h"

// outputvideo type flags
typedef enum {
	OUTPUT_VIDEO_BASIC = 0,
	OUTPUT_VIDEO_BLOB = 1,
	OUTPUT_VIDEO_MD = 2,
	OUTPUT_VIDEO_RAT = 4,
	OUTPUT_VIDEO_CAGE = 8, // deprecated
	OUTPUT_VIDEO_BARCODES = 16,
	OUTPUT_VIDEO_BARCODE_COLOR_LEGEND = 32,
	OUTPUT_VIDEO_PAIR_ID = 64,
	OUTPUT_VIDEO_VELOCITY = 128
} outputvideotype_t;

//! A structure for storing control states (that are read from the .ini file)
class cCS {
  public:
    // bools: debug and output
    bool bProcessText;
    bool bProcessImage;
    bool bShowVideo;
    bool bShowDebugVideo;
    int bWriteVideo;            // 0 - nothing;  1 - video+frames;  2 - only frames
    bool bWriteText;
    bool bCout;
    bool bCin;
    bool bApplyROIToVideoOutput;
    // motion detection filter
    bool bMotionDetection;
    double mdAlpha;
    int mdThreshold;
    int mdAreaMin;
    int mdAreaMax;
    // blob and ID detection
    int mRats;                  // max number of rats on the screen
    int mChips;                 // number of coloured blobs on each rat
    int mBase;                  // number of colours used
    int mDiaMin;                // minimum diameter of a colored blob [pixel]
    int mDiaMax;                // maximum diameter of a colored blob [pixel]
	double mAreaMin;            // max area calculated from max diameter (watch for sync)
	double mAreaMax;            // min area calculated from min diameter (watch for sync)
	double mElongationMax;      // maximum elongation of blob ellipse (A/B)
    bool bBlobE;                // blobs are treated as ellipses (1) or circles(0)?
    // dilate and erode operation params
	int mErodeBlob;
	int mDilateBlob;
	int mErodeRat;
	int mDilateRat;
	// day/night switch
    bool bLED;                  // do we use it at all or Day settings by default?
    CvPoint mLEDPos;            // X,Y coordinate of the red LED switch
    tColor mLEDColor;           // color of the LED blob
    // general
    char inifile[MAXPATH];      // this is not read from .ini file but defines it (can be overwritten from command line arg.)
    char paintdatefile[MAXPATH];
    char inputvideofile[MAXPATH];
    char inputbarcodefile[MAXPATH];     // trajognize style
    char inputdatfile[MAXPATH]; // output of previous ratognize run
    char inputlogfile[MAXPATH]; // output of previous ratognize run
    char outputfilecommon[MAXPATH];
    char outputvideofile[MAXPATH];
    char outputdatfile[MAXPATH];        // {} style
    char outputlogfile[MAXPATH];
    char outputdirectory[MAXPATH];
    int outputvideoskipfactor;
    int outputscreenshotskipfactor;
    int LEDdetectionskipfactor;
    timed_t hipervideostart;    // date for the hipervideo to start
    timed_t hipervideoend;      // date for the hipervideo to end
    int hipervideoduration;     // parsed to be in seconds
    outputvideotype_t outputvideotype;  // what goes on the output video?
    int firstframe;
    int lastframe;
    int displaywidth;
    CvRect imageROI;
    int dayssincelastpaint;     // colors are stored in an external list, here only day is specified since last paint
    color_interpolation_t colorselectionmethod;   // interpolate(0), fit_linear(1) or interpolate_date(2)
    int gausssmoothing;
    bool bInputVideoIsInterlaced;
    //! Constructor.
    cCS(): bProcessText(false), bProcessImage(false), bShowVideo(false),
            bShowDebugVideo(false), bWriteVideo(0), bWriteText(false),
            bCout(false), bCin(false), bApplyROIToVideoOutput(false),
            bMotionDetection(false), mdAlpha(0.1), mdThreshold(15),
            mdAreaMin(5000), mdAreaMax(10000),
            mRats(28), mChips(3), mBase(5), mDiaMin(10), mDiaMax(50),
            mAreaMin(10 * 10 / 4 * 3.141592), mAreaMax(50 * 50 / 4 * 3.141592),
            mElongationMax(10), bBlobE(false),
		    mErodeBlob(2), mDilateBlob(2), mErodeRat(4), mDilateRat(6),
            bLED(false),
            //mLEDPos(?), mLEDColor(?)
            outputvideoskipfactor(1), outputscreenshotskipfactor(1),
            LEDdetectionskipfactor(1),
            hipervideostart(0), hipervideoend(0), hipervideoduration(0),
            outputvideotype(OUTPUT_VIDEO_BASIC),
            firstframe(0), lastframe(0), displaywidth(0), 
			//imageROI(?),
            dayssincelastpaint(0), colorselectionmethod(COLOR_FIT_LINEAR),
            gausssmoothing(0),  bInputVideoIsInterlaced(false) {
        strcpy(inifile, "etc/configs/ratognize.ini");
        paintdatefile[0]=0;
        inputvideofile[0]=0;
        inputbarcodefile[0]=0;
        inputdatfile[0]=0;
        inputlogfile[0]=0;
        outputfilecommon[0]=0;
        outputvideofile[0]=0;
        outputdatfile[0]=0;
        outputlogfile[0]=0;
        outputdirectory[0]=0;
        imageROI.width = 0;
        imageROI.height = 0;
        imageROI.x = 0;
        imageROI.y = 0;
        memset(&mLEDPos, 0, sizeof(mLEDPos));
        memset(&mLEDColor, 0, sizeof(mLEDColor));
    }
    //! Destructor.
	~cCS() {
    }
};

/**
 * Read all settings from an ini file
 *
 * \param tempDSLP        days since last paint variable
 * \param cs              control states structure
 * \param mColorDataBase  the color database that is to be filled from the file
 * \param mColor          the destination list of colors with the 'bUse' flags updated
 *
 * \return true on success, false otherwise
 */
bool ReadIniFile(bool tempDSLP, cCS* cs,
        std::list<cColorSet>* mColorDataBase, cColor* mColor);

#endif
