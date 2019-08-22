#define _USE_MATH_DEFINES
#include <cmath>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "barcode.h"
#include "blob.h"
#include "color.h"
#include "datetime.h"
#include "input.h"
#include "ini.h"
#include "light.h"
#include "log.h"

// TODO: there is no much error check. Format should be correct
// parameters are needed not to overwrite command line parameters if given...
bool ReadIniFile(bool tempDSLP, cCS* cs,
        std::list<cColorSet>* mColorDataBase, cColor* mColor) {
    // open ini file
    std::ifstream ifs(cs->inifile);
    if (ifs.bad() || ifs.fail()) {
        LOG_ERROR("Could not open ini file.");
        return false;
    }
    // init variables
    std::string str;
    char cc[MAXPATH];
    int i, j, k, ii;
    float f;
    lighttype_t light = DAYLIGHT;
    int bNewday = 0;
    cColorSet tempccc;          // reset called on declaration
    cCS tempcs = *cs;

    // read file line by line
    while (!ifs.eof()) {
        getline(ifs, str);
        if (!str.length() || str[0] == '#') {
            continue;
        }
        // bools
        if (sscanf(str.data(), "bShowVideo=%d", &i) == 1) {
            tempcs.bShowVideo = (i == 1);
        } else if (sscanf(str.data(), "bProcessImage=%d", &i) == 1) {
            tempcs.bProcessImage = (i == 1);
        } else if (sscanf(str.data(), "bProcessText=%d", &i) == 1) {
            tempcs.bProcessText = (i == 1);
        } else if (sscanf(str.data(), "bShowDebugVideo=%d", &i) == 1) {
            tempcs.bShowDebugVideo = (i == 1);
        } else if (sscanf(str.data(), "bWriteVideo=%d", &i) == 1) {
            tempcs.bWriteVideo = i;
        } else if (sscanf(str.data(), "bWriteText=%d", &i) == 1) {
            tempcs.bWriteText = (i == 1);
        } else if (sscanf(str.data(), "bCout=%d", &i) == 1) {
            tempcs.bCout = (i == 1);
        } else if (sscanf(str.data(), "bCin=%d", &i) == 1) {
            tempcs.bCin = (i == 1);
        } else if (sscanf(str.data(), "bApplyROIToVideoOutput=%d", &i) == 1) {
            tempcs.bApplyROIToVideoOutput = (i == 1);

        // motion detection
        } else if (sscanf(str.data(), "bMotionDetection=%d", &i) == 1) {
            tempcs.bMotionDetection = (i == 1);
        } else if (sscanf(str.data(), "mdAlpha=%g", &f) == 1) {
            tempcs.mdAlpha = f;
        } else if (sscanf(str.data(), "mdThreshold=%d", &i) == 1) {
            tempcs.mdThreshold = i;
        } else if (sscanf(str.data(), "mdAreaMin=%d", &i) == 1) {
            tempcs.mdAreaMin = i;
        } else if (sscanf(str.data(), "mdAreaMax=%d", &i) == 1) {
            tempcs.mdAreaMax = i;

        // input files
        } else if (sscanf(str.data(), "paintdatefile=\"%[^\"]s\"", cc) == 1
                && !cs->paintdatefile[0]) {
            strcpy_s(tempcs.paintdatefile, MAXPATH, cc);
        } else if (sscanf(str.data(), "inputvideofile=\"%[^\"]s\"", cc) == 1
                && !cs->inputvideofile[0]) {
            strcpy_s(tempcs.inputvideofile, MAXPATH, cc);
        // output directory
        } else if (sscanf(str.data(), "outputdirectory=\"%[^\"]s\"", cc) == 1
                && !cs->outputdirectory[0]) {
            strcpy_s(tempcs.outputdirectory, MAXPATH, cc);
		// generals
        } else if (sscanf(str.data(), "mRats=%d", &i) == 1) {
            tempcs.mRats = i;
        } else if (sscanf(str.data(), "mChips=%d", &i) == 1) {
            tempcs.mChips = i;
        } else if (sscanf(str.data(), "mBase=%d", &i) == 1) {
            tempcs.mBase = i;
        } else if (sscanf(str.data(), "mDiaMin=%d", &i) == 1) {
            tempcs.mDiaMin = i;
			tempcs.mAreaMin = (double)tempcs.mDiaMin * tempcs.mDiaMin / 4 * M_PI;
        } else if (sscanf(str.data(), "mDiaMax=%d", &i) == 1) {
            tempcs.mDiaMax = i;
			tempcs.mAreaMax = (double)tempcs.mDiaMax * tempcs.mDiaMax / 4 * M_PI;
		} else if (sscanf(str.data(), "mElongationMax=%f", &f) == 1) {
            tempcs.mElongationMax = (double) f;
        } else if (sscanf(str.data(), "bBlobE=%d", &i) == 1) {
            tempcs.bBlobE = (i == 1);
		// dilate/erode
		} else if (sscanf(str.data(), "mErodeBlob=%d", &i) == 1) {
			tempcs.mErodeBlob = i;
		} else if (sscanf(str.data(), "mDilateBlob=%d", &i) == 1) {
			tempcs.mDilateBlob = i;
		} else if (sscanf(str.data(), "mErodeRat=%d", &i) == 1) {
			tempcs.mErodeRat = i;
		} else if (sscanf(str.data(), "mDilateRat=%d", &i) == 1) {
			tempcs.mDilateRat = i;
		// skip factors
		} else if (sscanf(str.data(), "outputvideoskipfactor=%d", &i) == 1) {
            tempcs.outputvideoskipfactor = std::max(i, 1);
        } else if (sscanf(str.data(), "outputscreenshotskipfactor=%d", &i) == 1) {
            tempcs.outputscreenshotskipfactor = std::max(i, 1);
        } else if (sscanf(str.data(), "LEDdetectionskipfactor=%d", &i) == 1) {
            tempcs.LEDdetectionskipfactor = std::max(i, 1);
        } else if (sscanf(str.data(), "outputvideotype=%d", &i) == 1) {
            tempcs.outputvideotype = (outputvideotype_t) i;
        } else if (sscanf(str.data(), "imageROI=%d %d %d %d", &i, &j, &k,
                &ii) == 4) {
            tempcs.imageROI.x = i;
            tempcs.imageROI.y = j;
            tempcs.imageROI.width = k;
            tempcs.imageROI.height = ii;
        } else if (sscanf(str.data(), "firstframe=%d", &i) == 1) {
            tempcs.firstframe = i;
        } else if (sscanf(str.data(), "lastframe=%d", &i) == 1) {
            tempcs.lastframe = i;
        } else if (sscanf(str.data(), "displaywidth=%d", &i) == 1) {
            tempcs.displaywidth = i;
        } else if (sscanf(str.data(), "gausssmoothing=%d", &i) == 1) {
            tempcs.gausssmoothing = i;
        } else if (sscanf(str.data(), "bInputVideoIsInterlaced=%d", &i) == 1) {
            tempcs.bInputVideoIsInterlaced = (i == 1);
        } else if (sscanf(str.data(), "colorselectionmethod=%d", &i) == 1) {
            tempcs.colorselectionmethod = (color_interpolation_t)i;

        // hipervideo
        } else if (sscanf(str.data(), "hipervideostart=%s", cc) == 1) {
			if (ParseDateTime(&tempcs.hipervideostart, cc) != 6) {
				break;
			}
        } else if (sscanf(str.data(), "hipervideoend=%s", cc) == 1) {
			if (ParseDateTime(&tempcs.hipervideoend, cc) != 6) {
				break;
			}
        } else if (sscanf(str.data(), "hipervideoduration=%s", cc) == 1) {
            // parse date and time
            timed_t temptime;
			if (ParseDateTime(&temptime, cc, true) != 6) {
				break;
			}
			tempcs.hipervideoduration = (int)temptime;
        // LED day/night switch
        } else if (sscanf(str.data(), "bLED=%d", &i) == 1) {
            tempcs.bLED = (i == 1);
        } else if (sscanf(str.data(), "mLEDPos=%d %d", &i, &j) == 2) {
            tempcs.mLEDPos.x = i;
            tempcs.mLEDPos.y = j;
        } else if (sscanf(str.data(), "mLEDColor=%d %d %d", &i, &j, &k) == 3) {
            tempcs.mLEDColor.mColorHSV = cvScalar(i, j, k);
        } else if (sscanf(str.data(), "mLEDRange=%d %d %d", &i, &j, &k) == 3) {
            tempcs.mLEDColor.mRangeHSV = cvScalar(i, j, k);

        // paint variable
        } else if (sscanf(str.data(), "dayssincelastpaint=%d", &i) == 1 &&
                !tempDSLP) {
            tempcs.dayssincelastpaint = i;

        // light conditions
        } else if (sscanf(str.data(), "DAYLIGHT=%s %d", cc, &i) == 2) {
            // parse date and time
            timed_t temptime;
			if (ParseDateTime(&temptime, cc) != 6) {
				break;
			}
            // store variables
            if (bNewday) {
                mColorDataBase[light].push_back(tempccc);
                bNewday = 0;
            }
            // Note that we keep background color with the 'false' argument
			tempccc.Reset(false);
            tempccc.day = i;
            tempccc.datetime = (time_t) temptime;
            light = DAYLIGHT;
        } else if (sscanf(str.data(), "NIGHTLIGHT=%s %d", cc, &i) == 2) {
            // parse date and time
            timed_t temptime;
			if (ParseDateTime(&temptime, cc) != 6) {
				break;
			}
            // store variables
            if (bNewday) {
                mColorDataBase[light].push_back(tempccc);
                bNewday = 0;
            }
            // Note that we keep background color with the 'false' argument
            tempccc.Reset(false);
            tempccc.day = i;
            tempccc.datetime = (time_t) temptime;
            light = NIGHTLIGHT;
        // colors (1-5 in file and display, 0-4 here)
        } else if (sscanf(str.data(), "mUse%d=%s %d", &ii, cc, &i) == 3) {
            mColor[ii].mUse = (i == 1);
            strcpy_s(mColor[ii].name, 10, cc);
        } else if (sscanf(str.data(), "mColorHSV%d=%d %d %d", &ii, &i, &j,
                &k) == 4) {
            tempccc.mColor[ii].mColorHSV = cvScalar(i, j, k);
            bNewday = 1;
        } else if (sscanf(str.data(), "mRangeHSV%d=%d %d %d", &ii, &i, &j,
                &k) == 4) {
            tempccc.mColor[ii].mRangeHSV = cvScalar(i, j, k);
            bNewday = 1;
        } else if (sscanf(str.data(), "mBGColorHSV=%d %d %d", &i, &j,
                &k) == 3) {
            tempccc.mBGColor.mColorHSV = cvScalar(i, j, k);
            bNewday = 1;
        } else if (sscanf(str.data(), "mBGRangeHSV=%d %d %d", &i, &j,
                &k) == 3) {
            tempccc.mBGColor.mRangeHSV = cvScalar(i, j, k);
            bNewday = 1;
        }

    }

    // release memory
    ifs.close();
    str.clear();

    // TODO: more thorough error checking
    if (!tempcs.mDiaMin || !tempcs.mDiaMax) {
        LOG_ERROR("Read error, some parameters are missing.");
        return false;
    }
    // check coexistence of image and text (.blobs, .blobs.barcodes, etc.) processing
    if (tempcs.bProcessImage && tempcs.bProcessText) {
        LOG_ERROR("Cannot process image and previously created text at the same time. TODO: implement.");
        return false;
    }
    // store last color and check error
	if (bNewday) {
		mColorDataBase[light].push_back(tempccc);
	}
    if (mColorDataBase[DAYLIGHT].empty() || mColorDataBase[NIGHTLIGHT].empty()) {
        LOG_ERROR("Read error, not enough colors specified; D: %zd, N: %zd",
                mColorDataBase[DAYLIGHT].size(),
                mColorDataBase[NIGHTLIGHT].size());
        return false;
    }
    // sort entries according to date
    if (tempcs.colorselectionmethod == 2) {
        mColorDataBase[DAYLIGHT].sort(compareCColorSetsByDate);
        mColorDataBase[NIGHTLIGHT].sort(compareCColorSetsByDate);
        // or day(sincelastpaint) (default)
    } else {
        mColorDataBase[DAYLIGHT].sort();
        mColorDataBase[NIGHTLIGHT].sort();
    }

    // store all read variables and return with no error
    *cs = tempcs;
    return true;
}

