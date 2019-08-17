#ifndef HEADER_INPUT
#define HEADER_INPUT

#include <iostream>
#include <fstream>
#include <sstream>

#include "ini.h"

/**
 * Opens an input file stream for reading.
 *
 * \param ifs       the input file stream used
 * \param filename  the name of the file to open
 *
 * \return true on success, false otherwise
 */
bool OpenInputFileStream(std::ifstream& ifs, char *filename);

/**
 * Read next line of a barcode file stream into a barcode structure.
 *
 * \param ifs        the barcode input file stream
 * \param mBarcodes  the barcode structure to store barcodes
 * \param cs         control states structure
 * \param currentframe  the current frame
 *
 * \return true on success, false otherwise
 */
bool ReadNextBarcodesFromFile(std::ifstream& ifs, tBarcode& mBarcodes, cCS* cs, int currentframe);

/**
 * Read next line of a blob file stream into blob structures.
 *
 * \param ifs             the blob input file stream
 * \param mBlobParticles  the blob structure to store colored blobs
 * \param mMDParticles    the blob structure to store motion blobs
 * \param mRatParticles   the blob structure to store rat blobs
 * \param cs              control states structure
 * \param currentframe    the current frame
 *
 * \return true on success, false otherwise
 */
bool ReadNextBlobsFromFile(std::ifstream& ifs, tBlob& mBlobParticles,
        tBlob& mMDParticles, tBlob& mRatParticles, cCS* cs, int currentframe);

/**
 * Read next line of a log file stream to parse light settings
 *
 * \param ifs     the log input file stream
 * \param mLight  the light structure where the parsed value will be stored
 * \param currentframe  the current frame
 *
 * \return -1 on failure, 0 on success, 1 on success if light has changed
 */
int ReadNextLightFromLogFile(std::ifstream& ifs, lighttype_t* mLight, int currentframe);

/**
 * Read list of dates when the rats have been repainted.
 *
 * \param mPaintDates  the destination list of time values parsed
 * \param cs           control sattes structure
 *
 * \return true on success, false otherwise
 */
bool ReadPaintDateFile(std::list <time_t>& mPaintDates, cCS* cs);

#endif