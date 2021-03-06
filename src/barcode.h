#ifndef HEADER_BARCODE
#define HEADER_BARCODE

#include <opencv2/opencv.hpp>
#include <vector>

#include "constants.h"

#define BARCODETAG ".barcodes"

// a barcode (only loaded from trajognize output in this version)
class cBarcode {
  public:
    char strid[MAXMBASE + 1];              // name id string
    cv::Point2d mCenter;       // barcode center [pixels].
    double mOrientation;        // The orientation angle of the particle [rad].
    double mAxisA;              // barcode major axis, assuming elliptical shape [pixel]
    double mAxisB;              // barcode minor axis, assuming elliptical shape [pixel]
    int mFix;                   // barcode mFix value (any values from mfix_t with logical OR relation)
    //! Constructor.
    cBarcode() {
        Reset();
    }
    //! Destructor.
	~cBarcode() {
    }
    //reset
    void Reset() {
        strid[0] = 0;
        mCenter.x = 0;
        mCenter.y = 0;
        mOrientation = 0;
        mAxisA = 0;
        mAxisB = 0;
        mFix = 0;
    }
};

// barcode vector type
typedef std::vector < cBarcode > tBarcode;

#endif
