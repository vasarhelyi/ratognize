#define _USE_MATH_DEFINES
#include <cmath>
#include <time.h>

#include "barcode.h"
#include "blob.h"
#include "output_text.h"


////////////////////////////////////////////////////////////////////////////////
// TODO: no error check yet
void WriteLogFileHeader(cCS* cs, std::string args, std::ofstream& ofslog) {
	// open and truncate log output file
	ofslog.open(cs->outputlogfile, std::ios::out | std::ios::trunc);
	// get time
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	// store ini file
	char cc[512];
	sprintf(cc,
		cs->bProcessText ? "cp %s %s%s" BARCODETAG ".ini" : "cp %s %s%s.ini",
		cs->inifile, cs->outputdirectory, cs->outputfilecommon);
	system(cc);                 // TODO: does it work on linux??? \,/ characters, etc.

								// write dat header
	ofslog << "# ratognize log file created on: " << asctime(timeinfo) << std::endl
		<< "# file was called like this: " << args << std::endl
		<< "# used ini file with settings is stored in: " << cs->
		outputfilecommon << ".ini" << std::endl << std::endl <<
		"# Log file format: frame warningtype params" << std::endl <<
		"# Log file entry types:" << std::endl <<
		"#   FIRSTFRAME/LASTFRAME -- frame number of first and last frame read from file"
		<< std::endl <<
		"#   LED newstate -- led state has changed (possible values: DAYLIGHT, NIGHTLIGHT, EXTRALIGHT, STRANGELIGHT)."
		<< std::endl <<
		"#   AVG avgR avgG avgB votes_for_daylight maxLEDblobsize -- average intensity of image channels + other params"
		<< std::endl <<
		"#   BLOBOVERSIZE color/MD/RAT num maxsize -- There are blobs greater than the maximum size allowed."
		<< std::endl <<
		"#   BLOBUNDERSIZE color/MD/RAT num -- There are blobs too small but larger than 80% of the minimum size allowed."
		<< std::endl << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// TODO: no error check yet
void WriteBlobFileHeader(cCS* cs, std::ofstream& ofsdat) {
    // open and truncate dat output file
    ofsdat.open(cs->outputdatfile, std::ios::out | std::ios::trunc);
    // write dat header
    if (cs->bMotionDetection)
        ofsdat <<
                "# MD line format (assuming ellipse): framenum\tMD\tMDBlobCount\t{centerx\tcentery\taxisA\taxisB\torientation}\t{..."
                << std::endl;
    ofsdat <<
            "# RAT line format (assuming ellipse): framenum\tRAT\tRATBlobCount\t{centerx\tcentery\taxisA\taxisB\torientation}\t{..."
            << std::endl;
    if (cs->bBlobE) {
        ofsdat <<
                "# BLOBE line format (assuming ellipse): framenum\tBLOB\tBlobCount\t{color\tcenterx\tcentery\taxisA\taxisB\torientation}\t{..."
                << std::endl;
    } else {
        ofsdat <<
                "# BLOB line format (assuming circle): framenum\tBLOB\tBlobCount\t{color\tcenterx\tcentery\tradius}\t{..."
                << std::endl;
    }
    // set output to 1 decimal digits. This is enough for everything if we convert orientation into degrees.
    ofsdat.setf(std::ios::fixed, std::ios::floatfield);
    ofsdat.precision(1);
}

////////////////////////////////////////////////////////////////////////////////
// TODO: no error check yet
// output coordinates are in total image coordinates, not ROI
void WriteBlobFile(cCS* cs, std::ofstream& ofsdat, tBlob& mBlobParticles,
        tBlob& mMDParticles, tBlob& mRatParticles, int currentframe) {
    tBlob::iterator it;

    /////////// dat ///////////

    // write MDBlobParticles
    if (cs->bMotionDetection) {
        ofsdat << currentframe << "\tMD\t" << (int) mMDParticles.size() << "\t";
        for (it = mMDParticles.begin(); it != mMDParticles.end(); ++it) {
            ofsdat << "{" << (*it).mCenter.x + cs->imageROI.x << "\t" << (*it).mCenter.y + cs->imageROI.y << "\t" << (*it).mAxisA << "\t" << (*it).mAxisB << "\t" << (*it).mOrientation * 180 / M_PI << "}\t";      // [deg]
        }
        ofsdat << std::endl;
    }
    // write RatParticles
    ofsdat << currentframe << "\tRAT\t" << (int) mRatParticles.size() << "\t";
    for (it = mRatParticles.begin(); it != mRatParticles.end(); ++it) {
        ofsdat << "{" << (*it).mCenter.x + cs->imageROI.x << "\t" << (*it).mCenter.y + cs->imageROI.y << "\t" << (*it).mAxisA << "\t" << (*it).mAxisB << "\t" << (*it).mOrientation * 180 / M_PI << "}\t";  // [deg]
    }
    ofsdat << std::endl;

    // write extended BlobParticles (as ellipse)
    if (cs->bBlobE) {
        ofsdat << currentframe << "\tBLOBE\t" << (int) mBlobParticles.
                size() << "\t";
        for (it = mBlobParticles.begin(); it != mBlobParticles.end(); ++it) {
            ofsdat << "{" << (*it).index << "\t" << (*it).mCenter.x + cs->imageROI.x << "\t" << (*it).mCenter.y + cs->imageROI.y << "\t" << (*it).mAxisA << "\t" << (*it).mAxisB << "\t" << (*it).mOrientation * 180 / M_PI << "}\t";       // [deg]
        }
        // write BlobParticles (as circles)
    } else {
        ofsdat << currentframe << "\tBLOB\t" << (int) mBlobParticles.
                size() << "\t";
        for (it = mBlobParticles.begin(); it != mBlobParticles.end(); ++it) {
            ofsdat << "{" << (*it).index << "\t"
                    << (*it).mCenter.x + cs->imageROI.x << "\t"
                    << (*it).mCenter.y + cs->imageROI.y << "\t"
                    << (*it).mRadius << "}\t";
        }
    }
    ofsdat << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// this function should be called once at the beginning
void OutputFadingColorsToFile(cCS* cs, std::list<cColorSet>* mColorDataBase, 
		cColor* mColor, tColor* mBGColor, lighttype_t mLight, 
		timed_t inputvideostarttime) {
    const char *hsv = "HSV";
    int i, j, k, kk, minday, maxday;
    lighttype_t light;
    std::ofstream fadefile;
    std::list < cColorSet >::iterator it, it2;
    int oldDSLP = cs->dayssincelastpaint;
    // open and truncate output file
    char cc[MAXPATH];
    sprintf(cc, "%s%s.colors", cs->outputdirectory, cs->outputfilecommon);
    fadefile.open(cc, std::ios::out | std::ios::trunc);

    // save params
    for (kk = 0; kk < 2; kk++) {    // 0: interpolate, 1: fit linear
        lighttype_t arr[2] = { DAYLIGHT, NIGHTLIGHT };
        for (k = 0; k < 2; k++) {
            light = arr[k];
            fadefile << "# " << lighttypename[light] << " color settings" <<
                    std::endl;
            fadefile << "# " << (kk ==
                    0 ? "METHOD:interpolate" : "METHOD:fit_linear") << std::endl;
            fadefile << "day\tfrom_date";
            for (i = 0; i < MAXMBASE; i++)
                for (j = 0; j < 3; j++)
                    fadefile << "\t" << mColor[i].name << "_" << hsv[j] << 
							"\t" << mColor[i].name << "_" << hsv[j] << "R";
            fadefile << std::endl;
            it = mColorDataBase[light].begin();
            minday = (*it).day;
            maxday = (*it).day;
            while (it != mColorDataBase[light].end()) {
                if ((*it).day < minday)
                    minday = (*it).day;
                if ((*it).day > maxday)
                    maxday = (*it).day;
                it++;
            }
            for (cs->dayssincelastpaint = minday;
                    cs->dayssincelastpaint <= maxday; cs->dayssincelastpaint++) {
                // TODO: error handling
                SetHSVDetectionParams(light, (color_interpolation_t)kk, 
						cs->dayssincelastpaint, inputvideostarttime, 
						mColorDataBase, mColor, mBGColor);
                // write day
                fadefile << cs->dayssincelastpaint;
                // write from_date
                it = mColorDataBase[light].begin();
                it2 = mColorDataBase[light].end();
                while (it != mColorDataBase[light].end()) {
                    if ((*it).day == cs->dayssincelastpaint) {
                        it2 = it;
                        break;
                    }
                    it++;
                }
                if (it2 == mColorDataBase[light].end())
                    fadefile << "\t-";
                else {
                    struct tm *timeinfo = localtime(&((*it2).datetime));
                    strftime(cc, MAXPATH, "\t%Y-%m-%d_%H-%M-%S", timeinfo);
                    fadefile << cc;
                }
                // write colors and ranges
                for (i = 0; i < MAXMBASE; i++)
                    for (j = 0; j < 3; j++)
                        fadefile << "\t" << mColor[i].mColor.mColorHSV.
                                val[j] << "\t" << mColor[i].mColor.mRangeHSV.
                                val[j];
                fadefile << std::endl;
            }
            fadefile << std::endl;
        }
    }
    // reset variables and close file
    cs->dayssincelastpaint = oldDSLP;
    // TODO: error handling
    SetHSVDetectionParams(mLight, cs->colorselectionmethod, 
			cs->dayssincelastpaint, inputvideostarttime, 
			mColorDataBase, mColor, mBGColor);
    fadefile.close();
}
