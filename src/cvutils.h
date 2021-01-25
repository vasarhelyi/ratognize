#ifndef HEADER_CVUTILS
#define HEADER_CVUTILS

#include <opencv2/opencv.hpp>

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
cv::Mat cvCreateImageOnce(cv::Mat &dst, cv::Size size, int depth,
        int channels, bool bZero=true);

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
void cvFilterHSV(cv::Mat &dstBin, cv::Mat &srcHSV, cv::Scalar colorHSV,
        cv::Scalar rangeHSV);

/**
 * Find the skeleton of an image.
 *
 * \param  src  the source image
 * \param  dst  the destination that contains the skeleton
 */
void cvSkeleton(cv::Mat &src, cv::Mat &dst);

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
void FilterMotion(cv::Mat &srcColor, cv::Mat &movingAverage,
        cv::Mat &dstGrey, double mdAlpha, int mdThreshold);

/**
 * Simple linear regression of y = a + b * x
 *
 * \param points  list of points on which we do a fitting
 * \param count   number of points in list
 * \param line    returned two parameters of the line
 */
void FitLine(cv::Point * points, int count, float *line);


#endif
