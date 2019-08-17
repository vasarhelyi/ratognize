#ifndef HEADER_CVUTILS
#define HEADER_CVUTILS

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

// positive orientation is mostly defined from right(x) towards down(y), as screen coordinates,
// but it is up(y)-position under linux in some older OpenCV versions.
// TODO: always check and synchronize with used OpenCV version,
// it might be buggy!!! Works for 2.3, 2.3.1, 2.4 stable releases
// under windows and 2.4.10 on atlasz under linux with Y_IS_DOWN = 1
#ifdef ON_LINUX
#define Y_IS_DOWN -1
#else
#define Y_IS_DOWN 1
#endif


/**
 * Allocates memory for an opencv image if it has not been allocated yet.
 *
 * \param  dst       the image pointer to be initialized
 * \param  size      size of the image
 * \param  depth     bit depth of the image
 * \param  channels  number of channels of the input image
 * \param  bZero     if True, image is zeroed after initialization
 *
 * \return the pointer to the new image
 */
IplImage *cvCreateImageOnce(IplImage * dst, CvSize size, int depth,
        int channels, bool bZero=true);

/**
 * Filter image with a given HSV color and range. This version is depreciated
 * as it is much slower than cvFilterHSV2.
 *
 * \param  dstBin    the destination binary image after filtering
 * \param  srcHSV    the input HSV image (8-bit)
 * \param  colorHSV  the color definition of the filter
 * \param  rangeHSV  the range definition of the filter
 */
void cvFilterHSV(IplImage * dstBin, IplImage * srcHSV, CvScalar * colorHSV,
        CvScalar * rangeHSV);

/**
 * Filter image with a given HSV color and range.
 *
 * source: http://www.shervinemami.co.cc/blobs.html
 *
 * \param  dstBin    the destination binary image after filtering
 * \param  srcHSV    the input HSV image (8-bit)
 * \param  colorHSV  the color definition of the filter
 * \param  rangeHSV  the range definition of the filter
 */
void cvFilterHSV2(IplImage * dstBin, IplImage * srcHSV, CvScalar * colorHSV,
        CvScalar * rangeHSV);

/**
 * Find the skeleton of an image.
 *
 * \param  src  the source image
 * \param  dst  the destination that contains the skeleton
 */
void cvSkeleton(CvArr * src, CvArr * dst);

/**
 * Filter for finding the dynamic part of an image.
 *
 * Source: http://sundararajana.blogspot.com/2007/05/motion-detection-using-opencv.html
 *
 * \param  srcColor       the newest input image
 * \param  movingAverage  the moving average of recent input images
 * \param  dstGrey        output image after the motion filter
 * \param  mdAlpha        the alpha parameter of the running average
 * \param  mdThreshold    the gray level threshold of the output
 */
void FilterMotion(IplImage * srcColor, IplImage * movingAverage,
        IplImage * dstGrey, double mdAlpha, int mdThreshold);

/**
 * Simple linear regression of y = a + b * x
 *
 * \param points  list of points on which we do a fitting
 * \param count   number of points in list
 * \param line    returned two parameters of the line
 */
void FitLine(CvPoint * points, int count, float *line);


#endif
