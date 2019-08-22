#ifndef HEADER_BLOB
#define HEADER_BLOB

#include <opencv2/opencv.hpp>
#include <vector>

#include "color.h"
#include "ini.h"

// a particle (or chip/bin/blob)
class cBlob {
  public:
    int index;                  // color index
    cv::Point2d mCenter;          //!< Particle center [pixels].
    double mOrientation;        //!< The orientation angle of the particle [rad].
    double mArea;               //!< Particle area [pixels^2].
    double mRadius;             //!< Particle radius, assuming circular shape [pixel]
    double mAxisA;              //!< Particle major axis, assuming elliptical shape [pixel]
    double mAxisB;              //!< Particle minor axis, assuming elliptical shape [pixel]
    //! Constructor.
    cBlob() {
        Reset();
    }
    //! Destructor.
	~cBlob() {
    }
    //reset
    void Reset() {
        index = 0;
        mCenter.x = 0;
        mCenter.y = 0;
        mOrientation = 0;
        mArea = 0;
        mRadius = 0;
        mAxisA = 0;
        mAxisB = 0;
    }
};

// particle/blob vector type
typedef std::vector < cBlob > tBlob;

/**
 * Fill a particle/blob structure from its moments.
 *
 * Sources:
 * wiki ellipse: http://en.wikipedia.org/wiki/Ellipse#Area
 * wiki image moments: http://en.wikipedia.org/wiki/Image_moments
 * opencv moments (WARNING: (15) is buggy): http://breckon.eu/toby/teaching/dip/opencv/SimpleImageAnalysisbyMoments.pdf
 *
 * \param particle  the blob class to be filled
 * \param moments   the calculated moments of the blob
 * \param bSkew     if true, skew parameters are also calculated
 */
void FillParticleFromMoments(cBlob* particle, cv::Moments &moments, bool bSkew);

/**
 * Finds all blobs on a binary image belonging to a given color.
 *
 * \param srcBin  the binary image on which blobs are to be found
 * \param i       the color index corresponding to the image
 * \param mColor  the color definition database
 * \param cs      control state structure
 * \param mBlobParticles  structure holding the found blobs
 * \param currentframe  the current video frame index
 * \param ofslog  output log file stream
 */
void FindSubBlobs(cv::Mat &srcBin, int i, cColor* mColor, cCS* cs,
        tBlob& mBlobParticles, int currentframe, std::ofstream& ofslog);

/**
 * Finds all blobs on a HSV image belonging to a given color.
 *
 * \param HSVimage    the HSV image on which blobs are to be found
 * \param i           the color index corresponding to the image
 * \param filterimage the filtered binary image containing blobs
                      (note that its output is also modified by
					  the contour finding method during processing)
 * \param mColor      the color definition database
 * \param cs          control state structure
 * \param mBlobParticles  structure holding the found blobs
 * \param currentframe  the current video frame index
 * \param ofslog      output log file stream
 *
 */
void FindHSVBlobs(cv::Mat &HSVimage, int i, cv::Mat &filterimage,
		cColor* mColor, cCS* cs,  tBlob& mBlobParticles,
		int currentframe, std::ofstream& ofslog);

/**
 * Finds motion / rat blobs on a binary image.
 *
 * \param srcBin        the binary image on which blobs are to be found
 * \param cs            control state structure
 * \param mParticles    structure holding the found blobs
 * \param currentframe  the current video frame index
 * \param ofslog        output log file stream
 *
 * Note that srcBin is modified due to the inner contour finding method.
 */
void FindMDorRatBlobs(cv::Mat &srcBin, cCS* cs, tBlob& mParticles,
		int currentframe, std::ofstream& ofslog);

/**
 * Filter background and return remaining image and its 'rat' blobs found.
 *
 * Name 'rat' comes from the fact that in the original experiment we had
 * rats to be found on the images.
 *
 * \param hsvimage      the HSV image on which background needs to be filtered
 * \param maskimage     the output binary mask image after filtering
 * \param mBGColor      the color definition of the background
 * \param cs            control state structure
 * \param mParticles    structure holding the found non-bacground blobs
 * \param currentframe  the current video frame index
 * \param ofslog        output log file stream
 *
 */
void DetectRats(cv::Mat &hsvimage, cv::Mat &maskimage, tColor* mBGColor,
		cCS* cs, tBlob& mParticles, int currentframe, std::ofstream& ofslog);

#endif
