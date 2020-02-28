#define _USE_MATH_DEFINES
#include <cmath>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "barcode.h"
#include "blob.h"
#include "color.h"
#include "datetime.h"
#include "input.h"
#include "light.h"
#include "log.h"

bool OpenInputFileStream(std::ifstream& ifs, char *filename) {
    ifs.open(filename);
    if (!ifs.is_open()) {
        return false;
    }
    return true;
}

bool ReadNextBarcodesFromFile(std::ifstream& ifs, tBarcode& mBarcodes, cCS* cs, int currentframe) {
    // init variables
    std::string line;
    cBarcode barcode;
    int i;
    // read file line by line
    while (!ifs.eof()) {
        // get next line
        getline(ifs, line);
        if (!line.length() || line[0] == '#')
            continue;
        // parse line
        std::stringstream lineStream(line);
        std::string token;
        // get frame number
        lineStream >> token;
        i = atoi(token.c_str());
		if (i < currentframe) {
			continue;
		} else if (i > currentframe) {
            LOG_ERROR("Barcode file frame No. mismatch: read number is too large (i=%d, currentframe=%d).", i, currentframe);
            return false;
        }
        // get blob count
        lineStream >> token;
        i = atoi(token.c_str());
        if (i < 0) {
            LOG_ERROR("Could not read number of barcodes from barcode file (column 2).");
            return false;
        }
        // if there are no barcodes, return without error and do not parse rest which is nothing
        if (i == 0)
            return true;
        // split rest of line into pieces: {ID  centerx centery xWorld  yWorld  orientation     mFix}   {...
        i = 0;
        while (lineStream >> token) {
            switch (i) {
                // ID
            case 0:
                strncpy(barcode.strid, token.c_str(), sizeof(cBarcode::strid));
                break;
                // centerx
            case 1:
                barcode.mCenter.x = atof(token.c_str()) - cs->imageROI.x;
                break;
                // centery
            case 2:
                barcode.mCenter.y = atof(token.c_str()) - cs->imageROI.y;
                break;
                // world coordinates not needed
            case 3:
            case 4:
                break;
                // orientation
            case 5:
                // convert back from [deg] to [rad]
                barcode.mOrientation = atof(token.c_str()) * M_PI / 180.0;
                break;
                // mFix
            case 6:
                barcode.mFix = atoi(token.c_str());
                break;
            }
            // check for full barcode structure
            if (++i == 7) {
                // calculate major and minor axis
                // TODO: this is not accurate if diameters are different for different blob colors
                barcode.mAxisB = (cs->mDiaMax[0] + cs->mDiaMin[0]) / 2 * 0.7;
                barcode.mAxisA = cs->mChips * barcode.mAxisB;
                mBarcodes.push_back(barcode);
                barcode.Reset();
                i = 0;
            }
        }
        // minimal error checking
        if (i) {
            LOG_ERROR("Invalid input format in " BARCODETAG " file.");
            return false;
        }
        // return without error after reading the proper line
        return true;
    }

    LOG_ERROR("No line left to read from " BARCODETAG " file for current frame.");
    return false;
}

bool ReadNextBlobsFromFile(std::ifstream& ifs, tBlob& mBlobParticles,
        tBlob& mMDParticles, tBlob& mRatParticles, cCS* cs, int currentframe) {
    // init variables
    std::string line;
    cBlob particle;
    int i, j;
    int linesread = 0;          // 1 - BLOB, 2 - MD, 4 - RAT
    if (!cs->bMotionDetection) {
        linesread = 2;          // we do not expect MD lines if motion detection is turned off
    }
    // read file line by line
    while (!ifs.eof()) {
        // get next line
        getline(ifs, line);
        if (!line.length() || line[0] == '#')
            continue;
        // parse line
        std::stringstream lineStream(line);
        std::string token, linetype;
        // get frame number
        lineStream >> token;
        i = atoi(token.c_str());
        if (i < currentframe)
            continue;
        else if (i > currentframe) {
            LOG_ERROR("Input blob file frame No. mismatch: read number is too large (i=%d, currentframe=%d)", i, currentframe);
            return false;
        }
        // get blob type
        lineStream >> linetype;
        // get blob number (NOP)
        lineStream >> token;
        if (linetype == "BLOB" || linetype == "BLOBE") {
            // BLOB line format has two variants:
            // (assuming circle): framenum  BLOB    BlobCount       {color centerx centery radius} {...
            // (assuming ellipse): framenum BLOB    BlobCount       {color centerx centery axisA axisB orientation} {...
            i = 0;
            j = 4;              // radius format (BLOB)
            while (lineStream >> token) {
                switch (i) {
                    // ID
                case 0:
                    particle.index = atoi(token.c_str() + 1);   // remove leading {
                    break;
                    // centerx
                case 1:
                    particle.mCenter.x = atof(token.c_str()) - cs->imageROI.x;
                    break;
                    // centery
                case 2:
                    particle.mCenter.y = atof(token.c_str()) - cs->imageROI.y;
                    break;
                    // radius / axisA
                case 3:
                    if (token.back() == '}') {
                        particle.mRadius = atof(token.c_str());
                    } else {
                        particle.mAxisA = atof(token.c_str());
                        j = 6;
                    }
                    break;
                    // axisB
                case 4:
                    particle.mAxisB = atof(token.c_str());
                    break;
                    // orientation
                case 5:
                    // convert back from [deg] to [rad]
                    particle.mOrientation = atof(token.c_str()) * M_PI / 180.0;
                    break;
                }
                // check for full particle structure
                if (++i == j) {
                    if (j == 4) {
                        particle.mAxisA = particle.mAxisB = particle.mRadius;
                    } else {
                        particle.mRadius =
                                sqrt(particle.mAxisA * particle.mAxisB);
                    }
                    mBlobParticles.push_back(particle);
                    particle.Reset();
                    i = 0;
                }
            }
            // minimal error checking
            if (i) {
                LOG_ERROR("Invalid input format in input blob file.");
                return false;
            }
            linesread |= 1;
        } else if (linetype == "MD" || linetype == "RAT") {
            // MD line format (assuming ellipse): framenum  MD      MDBlobCount     {centerx        centery axisA   axisB   orientation}    {...
            // RAT line format (assuming ellipse): framenum RAT     RATBlobCount    {centerx        centery axisA   axisB   orientation}    {...
            i = 0;
            while (lineStream >> token) {
                switch (i) {
                    // centerx
                case 0:
                    particle.mCenter.x = atof(token.c_str() + 1) - cs->imageROI.x;       // remove leading {
                    break;
                    // centery
                case 1:
                    particle.mCenter.y = atof(token.c_str()) - cs->imageROI.y;
                    break;
                    // axisA
                case 2:
                    particle.mAxisA = atof(token.c_str());
                    break;
                    // axisB
                case 3:
                    particle.mAxisB = atof(token.c_str());
                    break;
                    // orientation
                case 4:
                    // convert back from [deg] to [rad]
                    particle.mOrientation = atof(token.c_str()) * M_PI / 180.0;
                    break;
                }
                // check for full particle structure
                if (++i == 5) {
                    if (linetype == "MD")
                        mMDParticles.push_back(particle);
                    else
                        mRatParticles.push_back(particle);
                    particle.Reset();
                    i = 0;
                }
            }
            // minimal error checking
            if (i) {
                LOG_ERROR("Invalid input format in input blob file.");
                return false;
            }
            if (linetype == "MD")
                linesread |= 2;
            else
                linesread |= 4;
        } else {
            LOG_ERROR("Unknown line type in input blob file.");
            return false;
        }

        // return without error after reading all the proper lines
        if (linesread == 7)
            return true;
    }

    LOG_ERROR("No line left to read from input blob file for current frame.");
    return false;
}

int ReadNextLightFromLogFile(std::ifstream& ifs, lighttype_t* mLight, int currentframe) {
    // init variables
    static std::string prevline = "";
	std::string line;
	std::streampos lastpos;
    int i;
    // read file line by line
    while (!ifs.eof()) {
        // get next line (or continue with previous)
        if (prevline == "")
            getline(ifs, line);
        else
            line = prevline;
        // check for comment and empty lines
        if (!line.length() || line[0] == '#')
            continue;
        // parse line
		std::stringstream lineStream(line);
        std::string token, linetype;
        // get frame number
        lineStream >> token;
        i = atoi(token.c_str());
        if (i < currentframe)
            continue;
        else if (i > currentframe) {
            // use this line again next time for the next frame
            prevline = line;
            return 0;
        }
        // do not use previous line in the next call
        prevline = "";
        // get blob type
        lineStream >> linetype;
        // skip not LED lines
        if (linetype != "LED")
            continue;
        // get light type
        lineStream >> token;
        for (i = 0; i < 4; i++) {
            if (token == lighttypename[i]) {
                *mLight = (lighttype_t) i;
                return 1;
            }
        }
        LOG_ERROR("Unknown light mode read from log file: %s", token.c_str());
        return -1;
    }

    // TODO: when do we get here?
    return 0;
}

bool ReadPaintDateFile(std::list <time_t>& mPaintDates, cCS* cs) {
    // open paintdate file
	std::ifstream ifs(cs->paintdatefile);
    if (ifs.bad() || ifs.fail()) {
        LOG_ERROR("Could not open paintdate file.");
        return false;
    }
    // init variables
	std::string str;
    char cc[128];
    timed_t rawtime;
    mPaintDates.clear();

    // read file line by line
    while (!ifs.eof()) {
        getline(ifs, str);
        if (!str.length() || str[0] == '#') {
            continue;
        }

        // check line format and store if OK
        if (sscanf(str.data(), "PAINT %s", cc) == 1) {
            if (ParseDateTime(&rawtime, cc) == 6) {
                mPaintDates.push_back((time_t) rawtime);
            } else {
                LOG_ERROR("Error in PAINT line format. Check if not YYYY-MM-DD_hh-mm-ss.");
                return false;
            }
        }
    }

    // release memory
    ifs.close();
    str.clear();

    // error check
    if (mPaintDates.empty()) {
        LOG_ERROR("No dates have been read from paintdate file.");
        return false;
    }
    // sort entries in increasing order
    mPaintDates.sort();

    return true;
}
