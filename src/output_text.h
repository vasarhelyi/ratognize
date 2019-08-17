#ifndef HEADER_OUTPUT_TEXT
#define HEADER_OUTPUT_TEXT

#include <iostream>
#include <fstream>
#include <sstream>

#include "ini.h"

/**
 * Write log file header.
 *
 * \param cs      control states structure
 * \param args    arguments passed to main
 * \param ofslog  the output log file stream
 */
void WriteLogFileHeader(cCS* cs, std::string args, std::ofstream& ofslog);

/**
 * Write blob file header.
 *
 * \param cs      control states structure
 * \param ofsdat  the output data file stream
 */
void WriteBlobFileHeader(cCS* cs, std::ofstream& ofsdat);

/**
 * Write data accumulated at a given frame to blob file.
 *
 * \param cs      control states structure
 * \param ofsdat  the output data file stream
 * \param mBlobParticles  the blob structure to store colored blobs
 * \param mMDParticles    the blob structure to store motion blobs
 * \param mRatParticles   the blob structure to store rat blobs
 * \param currentframe    the current frame
 */
void WriteBlobFile(cCS* cs, std::ofstream& ofsdat, tBlob& mBlobParticles,
        tBlob& mMDParticles, tBlob& mRatParticles, int currentframe);

/**
 * Write fading color interpolation data to file.
 *
 * \param cs              control states structure
 * \param mColorDataBase  the color database that stores all color defs
 * \param mColor          the list of colors used currently
 * \param mBGColor        the background color used currently
 * \param mLight          the light structure used currently
 * \param inputvideostarttime  the starting time of the input video
 */
void OutputFadingColorsToFile(cCS* cs, std::list<cColorSet>* mColorDataBase, 
		cColor* mColor, tColor* mBGColor, lighttype_t mLight, 
		timed_t inputvideostarttime);

#endif