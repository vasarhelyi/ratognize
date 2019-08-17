#ifndef HEADER_BARCODE
#define HEADER_BARCODE

#include <vector>

#include <cv.h>
#include <cxcore.h>

#define BARCODETAG ".barcodes"

// a barcode (only loaded from trajognize output in this version)
class cBarcode {
  public:
    char strid[4];              // name id string
    CvPoint2D64f mCenter;       // barcode center [pixels].
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
