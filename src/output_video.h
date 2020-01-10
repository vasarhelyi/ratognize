#ifndef HEADER_OUTPUT_VIDEO
#define HEADER_OUTPUT_VIDEO

#include <iostream>
#include <fstream>
#include <sstream>

#include <opencv2/opencv.hpp>

#include "ini.h"


/**
 * Init fonts and videowriter for visual output.
 *
 * \param  cs            control states structure
 * \param  framesize     input frame size
 * \param  framesizeROI  the size of the output frame to be used.
 * \param  fps           frames per second of input/output video
 * \param  inputvideostarttime  time of the start of the input video
 *
 * Function does not return anything but initializes
 * font, narrowfont, copyrightfont and videowriter variables.
 */
void InitVisualOutput(cCS* cs, cv::Size framesize, cv::Size framesizeROI,
	double fps, timed_t inputvideostarttime);

/**
 * Destroys structures related to the video output.
 */
void DestroyVisualOutput();

/**
 * Sets hipervideoframestart and hipervideoframeend
 * based on .ini file and input video file name.
 *
 * Note that this function is called by InitVisualOutput().
 *
 * \param  cs            control states structure
 * \param  inputvideostarttime  time of the start of the input video
 * \param  fps           frames per second of input/output video
 *
 * \return true on success
 */
bool SetHiperVideoParams(cCS* cs, timed_t inputvideostarttime, double fps);

/**
 * Generate and write out all kinds of visual output.
 *
 * \param inputimage        the original input image
 * \param cs                control states structure
 * \param mBlobParticles    the blob structure to store colored blobs
 * \param mMDParticles      the blob structure to store motion blobs
 * \param mRatParticles     the blob structure to store rat blobs
 * \param mBarcodes         the barcode structure read back from files
 * \param mColor            the currently used color configuration
 * \param mLight            the currently used light configuration
 * \param inputvideostarttime  the starting time of the input video
 * \param currentframe      the current frame of the input video
 * \param the frame size of the input video
 * \param  framesizeROI  the size of the output frame to be used.
 * \param fps the frame per second setting of the input video
 */
void WriteVisualOutput(cv::Mat &inputimage, cCS* cs,
	tBlob& mBlobParticles, tBlob& mMDParticles, tBlob& mRatParticles,
	tBarcode& mBarcodes, cColor* mColor, lighttype_t mLight,
	timed_t inputvideostarttime, int currentframe, cv::Size framesize,
	cv::Size framesizeROI, double fps);

#endif
