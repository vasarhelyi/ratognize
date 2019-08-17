#define _USE_MATH_DEFINES
#include <cmath>

#include "blob.h"
#include "cvutils.h"


void FillParticleFromMoments(cBlob* particle, CvMoments* moments, bool bSkew) {
    // compute area, center and radius, assuming circular shape
    particle->mArea = moments->m00;
    particle->mRadius = sqrt(particle->mArea / M_PI);
    particle->mCenter.x = moments->m10 / moments->m00;
    particle->mCenter.y = moments->m01 / moments->m00;

    // compute orientation from moments
    // coordinate system with angle starting CW from x to y:
    //
    //  +-------- x
    //  |
    //  |
    //  |y
    //
    ////////////////
    particle->mOrientation = atan2(2 * moments->mu11, moments->mu20 - moments->mu02) / 2;       // [-pi/2,pi/2] range

    // compute major and minor axis assuming elliptical shape
    double d1 = (moments->mu20 + moments->mu02) / 2;
    double d2 = sqrt(4 * moments->mu11 * moments->mu11 + (moments->mu20 -
                    moments->mu02) * (moments->mu20 - moments->mu02)) / 2;
    double x = sqrt((d1 - d2) / (d1 + d2));     // elongation
    particle->mAxisA = sqrt(particle->mArea / (M_PI * x));
    particle->mAxisB = particle->mAxisA * x;

    if (bSkew) {
        // compute skewness along x and y
        double sx = moments->mu30 / pow(moments->mu20, 1.5);
        double sy = moments->mu03 / pow(moments->mu02, 1.5);
        // transform to axisA/axisB
        double sA = sx * cos(particle->mOrientation) +
                    sy * sin(Y_IS_DOWN * particle->mOrientation);
        //double sB = sx * sin(Y_IS_DOWN * particle->mOrientation) +
        //            sy * cos(particle->mOrientation);
        // correct orientation based on skew along main axis
        // (fish head is thicker than tail)
        if (sA > 0)
            particle->mOrientation += M_PI;
    }
}

void FindSubBlobs(IplImage* srcBin, int i, cColor* mColor, cCS* cs,
		tBlob& mBlobParticles, int currentframe, std::ofstream& ofslog) {
    double maxsize = cs->mAreaMin;
	double minsize = cs->mAreaMax;
    int overmaxcount = 0;
    int undermincount = 0;

    // Init blob extraction
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvContourScanner blobs = cvStartFindContours(srcBin, storage,
            sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    // init variables
    CvSeq *contour;
    CvMoments moments;

    // Iterate over first blobs (allow for more than final number, but not infinitely)
    while (mColor[i].mNumBlobsFound < cs->mRats * 20) {
        // Get next contour (if one exists)
        contour = cvFindNextContour(blobs);
        if (!contour)
            break;

        // Compute the moments
        cvMoments(contour, &moments);

        // store properly sized and shaped particles
        if (moments.m00 >= cs->mAreaMin && moments.m00 <= cs->mAreaMax) {
            // Compute particle parameters
            cBlob newparticle;
            FillParticleFromMoments(&newparticle, &moments, cs->bBlobE);

            // check elongation
            if (newparticle.mAxisB && newparticle.mAxisA / newparticle.mAxisB <=
                    cs->mElongationMax) {
                // set ID and increase blobnum
                mColor[i].mNumBlobsFound++;
                newparticle.index = i;

                // insert particle to common blobparticle list
                mBlobParticles.push_back(newparticle);
            }
        }
        // too big
        else if (moments.m00 > cs->mAreaMax) {
            overmaxcount++;
            if (moments.m00 > maxsize)
                maxsize = moments.m00;
        }
        // too small (but larger than one pixel)
        else if (moments.m00 > cs->mAreaMin * 0.8) {
            undermincount++;
            if (moments.m00 < minsize)
                minsize = moments.m00;
        }
        // Release the contour
        cvRelease((void **) &contour);
    }

    // Finalize blob extraction
    cvEndFindContours(&blobs);
    cvReleaseMemStorage(&storage);

    // write to log file, if needed
    if (overmaxcount) {
        ofslog << currentframe << "\tBLOBOVERSIZE\tc" << i << "-" << mColor[i].
                name << "\t" << overmaxcount << "\t" << maxsize << std::endl;
    }
    if (undermincount) {
        ofslog << currentframe << "\tBLOBUNDERSIZE\tc" << i << "-" << mColor[i].
                name << "\t" << undermincount << std::endl;  //<< "\t" << minsize << endl;
    }
}

void FindHSVBlobs(IplImage * HSVimage, int i, IplImage * filterimage,
		cColor* mColor, cCS* cs,  tBlob& mBlobParticles,
		int currentframe, std::ofstream& ofslog) {
	
	char cc[16];
	// filter with current HSV color into filterimage
    cvFilterHSV2(filterimage, HSVimage, &mColor[i].mColor.mColorHSV,
            &mColor[i].mColor.mRangeHSV);
	if (cs->mDilateBlob) {
		cvDilate(filterimage, filterimage, NULL, cs->mDilateBlob);
	}
	if (cs->mErodeBlob) {
		cvErode(filterimage, filterimage, NULL, cs->mErodeBlob);
	}
	// show debug images before they are modified by the contour finding method
	if (cs->bShowDebugVideo) {
		sprintf(cc, "c%d-%s", i, mColor[i].name);
		cvShowImage(cc, filterimage);
	}
    FindSubBlobs(filterimage, i, mColor, cs, mBlobParticles, currentframe, ofslog);
}

void FindMDorRatBlobs(IplImage * srcBin, cCS* cs, tBlob& mParticles, 
		int currentframe, std::ofstream& ofslog) {
    double maxsize = cs->mAreaMin;
    double minsize = cs->mAreaMax;
    int overmaxcount = 0;
    int undermincount = 0;
    // Init blob extraction
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvContourScanner blobs =
            cvStartFindContours(srcBin, storage, sizeof(CvContour),
            CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

    // init variables
    CvSeq *contour;
    CvMoments moments;

    // Iterate over blobs (no need to have more than ID's)
    while ((int) mParticles.size() < cs->mRats * 2) {
        // Get next contour (if one exists)
        contour = cvFindNextContour(blobs);
        if (!contour)
            break;

        // Compute the moments
        cvMoments(contour, &moments);

        // store properly sized particles
		// TODO: separate size restriction for RAT and MD?
        if (moments.m00 >= cs->mdAreaMin && moments.m00 <= cs->mdAreaMax) {
            // Compute particle parameters
            cBlob newparticle;
            FillParticleFromMoments(&newparticle, &moments, cs->bBlobE);

            // reset unused variables
            newparticle.index = 0;

            // insert particle to common blobparticle list
            mParticles.push_back(newparticle);
        }
        // too big
        else if (moments.m00 > cs->mdAreaMax) {
            overmaxcount++;
            if (moments.m00 > maxsize)
                maxsize = moments.m00;
        }
        // too small (but larger than half of min size)
        else if (moments.m00 > cs->mdAreaMin * 0.8) {
            undermincount++;
            if (moments.m00 < minsize)
                minsize = moments.m00;
        }
        // Release the contour
        cvRelease((void **) &contour);
    }

    // Finalize blob extraction
    cvEndFindContours(&blobs);
    cvReleaseMemStorage(&storage);

    // write to log file, if needed
    if (overmaxcount)
        ofslog << currentframe << "\t" << "BLOBOVERSIZE\tMD\t" << overmaxcount
                << "\t" << maxsize << std::endl;
    if (undermincount)
        ofslog << currentframe << "\t" << "BLOBUNDERSIZE\tMD\t" << undermincount << std::endl; //"\t" << minsize << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// filter backgroud and get only high saturation and different hue rat blobs 
void DetectRats(IplImage * hsvimage, IplImage * maskimage, tColor* mBGColor,
	cCS* cs, tBlob& mParticles, int currentframe, std::ofstream& ofslog) {
	// init variables
	static IplImage *binary = NULL;
	binary = cvCreateImageOnce(binary, cvGetSize(hsvimage), IPL_DEPTH_8U, 1, false);    // no need to zero it
	// detect background and invert it to detect rats as white blobs
	cvFilterHSV2(binary, hsvimage, &mBGColor->mColorHSV, &mBGColor->mRangeHSV);
	cvNot(binary, binary);
	// filter noise and possibly enlarge rat blobs
	if (cs->mErodeRat) {
		cvErode(binary, binary, 0, cs->mErodeRat);
	}
	if (cs->mDilateRat) {
		cvDilate(binary, binary, 0, cs->mDilateRat);
	}
    // debug show
    if (cs->bShowDebugVideo)
        cvShowImage("rats", binary);
    // copy to mask image
    cvCopy(binary, maskimage);
    // find rat blobs
    FindMDorRatBlobs(binary, cs, mParticles, currentframe, ofslog);
    // release memory
    //cvReleaseImage(&binary);
}
