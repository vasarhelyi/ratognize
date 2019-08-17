#define _USE_MATH_DEFINES
#include <cmath>
#include <time.h>

#include "barcode.h"
#include "blob.h"
#include "cvutils.h"
#include "log.h"
#include "mfix.h"
#include "output_video.h"

#include <opencv2/imgproc.hpp>


#define COPYRIGHTSTRING "(c) ELTE Department of Biological Physics, RATLAB "

// fonts, frame sizes and video writer
static CvFont font, narrowfont, copyrightfont;
static CvSize copyrightTextSize;
static CvVideoWriter *videowriter;
static IplImage *outputimage8x;        // copy of ROI area, the image that will be writte to videowriter
static CvSize framesize8x;             // if framesize is not multiples of 8, it will be adjusted to save video without error
static bool bFramesizeMismatch = false;
// hipervideo parameters
static int hipervideoframestart;
static int hipervideoframeend;
static int hipervideoskipfactor;
// pair measurement variables
char pair_str[2][8];

void InitVisualOutput(cCS* cs, CvSize framesize, CvSize framesizeROI, 
		double fps, timed_t inputvideostarttime) {
	std::ostringstream outfile;
	int i;

	// init standard fonts
	cvInitFont(&font, CV_FONT_VECTOR0, 0.8, 0.8, 0, 2);
	cvInitFont(&narrowfont, CV_FONT_VECTOR0, 0.8, 0.8, 0, 1);
	// init copyright font
	i = 20;
	char ccc[] = COPYRIGHTSTRING "0000-00-00 00-00-00.000 #99999 (NIGHT)";
	copyrightTextSize = framesizeROI;
	while (copyrightTextSize.width >= framesizeROI.width) {
		cvInitFont(&copyrightfont, CV_FONT_VECTOR0, (float)i / 10,
			(float)i / 10, 0, (i < 7 ? 1 : 2));
		cvGetTextSize(ccc, &copyrightfont, &copyrightTextSize, 0);
		i--;
	}
	std::cout << "Proper copyright string font size: " << (i + 1) / 10.0 << std::endl;

	// define pair measurement IDs from file name
	// assuming that filename ends with XXX_YYY.avi where XXX and YYY are the two strids (e.g. RBP)
	// if input video is not a pair measurement, we do not use pair_str anyways,
	// so it does not matter what it contains
	outfile.str("");
	outfile << cs->outputfilecommon;
	i = (int)outfile.str().rfind('.');
	memcpy(&pair_str[0][0], &cs->outputfilecommon[i - 7], 3);
	memcpy(&pair_str[1][0], &cs->outputfilecommon[i - 3], 3);

	// increase output image size if input is not multiples of 8
	framesize8x = framesize;
	bFramesizeMismatch = false;
	i = framesize.height % 8;
	if (i) {
		framesize8x.height += 8 - i;
		bFramesizeMismatch = true;
	}
	i = framesize.width % 8;
	if (i) {
		framesize8x.width += 8 - i;
		bFramesizeMismatch = true;
	}
	if (bFramesizeMismatch) {
		std::cout << "  WARNING: input framesize mismatch, height or width should be multiples of 8. Writing output will be slower." << std::endl;
		outputimage8x = cvCreateImage(framesize8x, IPL_DEPTH_8U, 3);    // output image with corrected size
	}

	// init output video with ROI or non-ROI framesize
	if (cs->bWriteVideo) {
#ifdef ON_LINUX
		// use ROI framesize
		if (cs->bApplyROIToVideoOutput) {
			videowriter = cvCreateVideoWriter(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesizeROI);
			// use non-ROI framesize
		}
		else {
			videowriter = cvCreateVideoWriter(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesize8x);
		}
#else
		// use ROI framesize
		if (cs->bApplyROIToVideoOutput) {
			videowriter = cvCreateVideoWriter(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesizeROI);
			//        if (cs->bApplyROIToVideoOutput) videowriter=cvCreateVideoWriter(cs->outputvideofile,0,fps,framesizeROI); // no compression
			// use non-ROI framesize
		}
		else {
			videowriter = cvCreateVideoWriter(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesize8x);
		}
		//              else videowriter=cvCreateVideoWriter(cs->outputvideofile,0,fps,framesize8x);  // no compression
#endif
	}
	else {
		videowriter = NULL;
	}

	// set hiper video params
	SetHiperVideoParams(cs, inputvideostarttime, fps);
}

void DestroyVisualOutput() {
	if (videowriter) {
		cvReleaseVideoWriter(&videowriter);
		videowriter = NULL;
	}
}


void WriteVisualOutput(IplImage *inputimage, IplImage *smoothinputimage, cCS* cs, 
		tBlob& mBlobParticles, tBlob& mMDParticles, tBlob& mRatParticles, 
		tBarcode& mBarcodes, cColor* mColor, lighttype_t mLight,
		timed_t inputvideostarttime, int currentframe, CvSize framesize, double fps) {
    //////////////////////////////////////////////////////
    // debug: do not comment out for determining min and max blob sizes
    //cvPutText(inputimage,"dia: cs->mDiaMin, dist: mDistMax",cvPoint(20+2*cs->mDistMax,20),&font,cvScalar(255,255,255));
    //cvCircle(inputimage,cvPoint(20,20),mDiaMin/2,cvScalar(255,255,255),2,CV_AA);
    //cvCircle(inputimage,cvPoint(20+cs->mDistMax,20),cs->mDiaMin/2,cvScalar(255,255,255),2,CV_AA);
    //cvPutText(inputimage,"dia: cs->mDiaMax, dist: mDistMax",cvPoint(20+2*cs->mDistMax,20+cs->mDistMax),&font,cvScalar(255,255,255));
    //cvCircle(inputimage,cvPoint(20,20+cs->mDistMax),cs->mDiaMax/2,cvScalar(255,255,255),2,CV_AA);
    //cvCircle(inputimage,cvPoint(20+cs->mDistMax,20+cs->mDistMax),cs->mDiaMax/2,cvScalar(255,255,255),2,CV_AA);
    //////////////////////////////////////////////////////

    tBlob::iterator it;
    CvPoint tmppoint;
    CvScalar color, color2;
    int dA, dB;
    //CvSize textSize;
    char cc[3];
    cc[1] = 0;

    // OUTPUT_VIDEO_MD debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_MD) {
        // plot MDParticles to image with black ellipses
        for (it = mMDParticles.begin(); it != mMDParticles.end(); ++it) {
            tmppoint.x = (int) (*it).mCenter.x;
            tmppoint.y = (int) (*it).mCenter.y;
            color = CV_RGB(50, 50, 50);     // DARK GREY
            cvEllipse(inputimage, tmppoint, cvSize((int) (*it).mAxisA,
                            (int) (*it).mAxisB),
                    Y_IS_DOWN * (*it).mOrientation * 180 / M_PI, 0, 360,
                    color, 2);
        }
    }
    // OUTPUT_VIDEO_RAT debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_RAT) {
        // plot RatParticles to image with gray ellipses
        dA = 0; // project fish might require 10
        dB = 0; // project fish might require 5
        for (it = mRatParticles.begin(); it != mRatParticles.end(); ++it) {
            tmppoint.x = (int) (*it).mCenter.x;
            tmppoint.y = (int) (*it).mCenter.y;
            color = CV_RGB(150, 150, 150);  // GRAY
            cvEllipse(inputimage, tmppoint,
                    cvSize((int)(*it).mAxisA + dA, (int)(*it).mAxisB + dB),
                    Y_IS_DOWN * (*it).mOrientation * 180 / M_PI, 0, 360,
                    color, 2);
            // draw orientation "arrow" as well
            color = CV_RGB(50, 50, 50); // DARKER GRAY
            cvEllipse(inputimage, tmppoint,
                    cvSize((int)(*it).mAxisA + dA, (int)(*it).mAxisB + dB),
                    Y_IS_DOWN * (*it).mOrientation * 180 / M_PI, -30, 30,
                    color, 2);
        }
    }
    // OUTPUT_VIDEO_BLOB debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_BLOB) {
        // plot BlobParticles to image with GREYSCALE brush
        for (it = mBlobParticles.begin(); it != mBlobParticles.end(); ++it) {
            cc[0] = mColor[(*it).index].name[0];    // zero based index
            tmppoint.x = (int) (*it).mCenter.x;
            tmppoint.y = (int) (*it).mCenter.y;
            color = CV_RGB(50, 50, 50);     // DARK GREY
            //cvCircle(inputimage,tmppoint,(int)(*it).mRadius,color,2);
            //cvEllipse(inputimage,tmppoint,cvSize((int)(*it).mAxisA,(int)(*it).mAxisB),
            //        Y_IS_DOWN*(*it).mOrientation*180/M_PI,0,360,color,2);
            //cvGetTextSize(cc, &font, &textSize,0);
            //tmppoint.x -= textSize.width/2;
            //tmppoint.y += textSize.height/2;
            color = CV_RGB(255, 255, 255);  // WHITE
            cvPutText(inputimage, cc, tmppoint, &font, color);
        }
    }
    // OUTPUT_VIDEO_BARCODES debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_BARCODES) {
        static const int step = 5;
        static tBarcode oldBarcodes[step];

        // plot Barcodes to image with colorful ellipses
        for (tBarcode::iterator itb = mBarcodes.begin();
                itb != mBarcodes.end(); ++itb) {
            tmppoint.x = (int) (*itb).mCenter.x;
            tmppoint.y = (int) (*itb).mCenter.y;
            // define ellipse color
            if ((*itb).mFix & MFIX_FULLFOUND) {
                color = CV_RGB(255, 0, 0);
                color2 = color;
            }               // RED if full OK
            if ((*itb).mFix & MFIX_FULLNOCLUSTER) {
                color = CV_RGB(128, 0, 0);
                color2 = color;
            }               // DARK RED if full OK and no cluster
            if ((*itb).mFix & MFIX_PARTLYFOUND_FROM_TDIST) {
                color = CV_RGB(255, 128, 128);
                color2 = color;
            }               // PINK if only partly found
            if (!(*itb).mFix || ((*itb).mFix & MFIX_DELETED)) {
                color = CV_RGB(0, 0, 0);
                color2 = color;
            }               // BLACK if deleted
            if ((*itb).mFix & MFIX_CHANGEDID) {
                color = CV_RGB(128, 255, 128);
                color2 = color;
            }               // LIGHT GREEN if deleted+changedid
            if ((*itb).mFix & MFIX_CHOSEN) {
                if ((*itb).mFix & MFIX_VIRTUAL) {
                    color = CV_RGB(255, 255, 0);
                    color2 = color;
                }           // YELLOW if chosen and virtual, final
                else {
                    color = CV_RGB(255, 255, 255);
                    color2 = color;
                }           // WHITE if chosen, final
            }
            if ((*itb).mFix & MFIX_DEBUG) {
                color = CV_RGB(128, 0, 128);
            }               // PURPLE (ellipse color) if debug

            // define different text color for shares*
            if (!((*itb).mFix & MFIX_DELETED)
                    && ((*itb).mFix & MFIX_SHARESBLOB
                            || (*itb).mFix & MFIX_SHARESID)) {
                color2 = CV_RGB(50, 0, 0);
                if ((*itb).mFix & MFIX_SHARESBLOB) {        // add GREEN if particles bin's are used in more ID's
                    color2.val[1] = 255;    // BGR
                }
                if ((*itb).mFix & MFIX_SHARESID) {  // add BLUE if there are more particles with same ID
                    color2.val[0] = 255;    // BGR
                }
                // becomes LIGHT BLUE if both above
            }
            cvEllipse(inputimage, tmppoint, cvSize((int) (*itb).mAxisA,
                            (int) (*itb).mAxisB),
                    Y_IS_DOWN * (*itb).mOrientation * 180 / M_PI, 0, 360,
                    color, (!(*itb).mFix
                            || ((*itb).mFix & MFIX_DELETED)) ? 1 : 2);
            // write barcode string as well
            tmppoint.x += (int) ((*itb).mAxisA * cos((*itb).mOrientation));
            tmppoint.y +=
                    (int) ((*itb).mAxisA * sin(Y_IS_DOWN *
                            (*itb).mOrientation));
            cvPutText(inputimage, (*itb).strid, tmppoint, (!(*itb).mFix
                            || ((*itb).
                                    mFix & MFIX_DELETED)) ? (&narrowfont)
                    : (&font), color2);

            // show barcode velocity if needed
            if (cs->outputvideotype & OUTPUT_VIDEO_VELOCITY) {
                for (tBarcode::iterator itoldb = oldBarcodes[0].begin();
                        itoldb != oldBarcodes[0].end(); ++itoldb) {
                    if (((*itb).mFix & MFIX_CHOSEN)
                            && ((*itoldb).mFix & MFIX_CHOSEN)
                            && strcmp((*itb).strid, (*itoldb).strid) == 0) {
                        int vel =
                                (int) hypot((*itb).mCenter.x -
                                (*itoldb).mCenter.x,
                                (*itb).mCenter.y - (*itoldb).mCenter.y);
                        char tempstr[256];
                        sprintf(tempstr, ",%d", vel);
                        tmppoint.x += 50;
                        cvPutText(inputimage, tempstr, tmppoint, &font,
                                color);
                    }
                }
            }
        }
        int xxx;
        for (xxx = 0; xxx < step - 1; xxx++)
            oldBarcodes[xxx] = oldBarcodes[xxx + 1];
        oldBarcodes[xxx] = mBarcodes;
    }
    // OUTPUT_VIDEO_BARCODE_COLOR_LEGEND debug output
    // Warning: make sure to be consistent with OUTPUT_VIDEO_BARCODES part
    if (cs->outputvideotype & OUTPUT_VIDEO_BARCODE_COLOR_LEGEND) {
        // text starts at top left of ROI image, or original image?
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvResetImageROI(inputimage);

        char tempstr[256];
        tmppoint.x = 25;
        tmppoint.y = 40;
        // color
        // fullfound: RED
        sprintf(tempstr, "FULLFOUND");
        color = CV_RGB(255, 0, 0);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // fullnocluster: DARK RED
        sprintf(tempstr, "FULLNOCLUSTER");
        color = CV_RGB(128, 0, 0);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // partlyfound: PINK
        sprintf(tempstr, "PARTLYFOUND_FROM_TDIST");
        color = CV_RGB(255, 128, 128);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // chosen: WHITE
        sprintf(tempstr, "CHOSEN");
        color = CV_RGB(255, 255, 255);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // virtual+chosen: YELLOW
        sprintf(tempstr, "VIRTUAL");
        color = CV_RGB(255, 255, 0);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // deleted: BLACK narrow
        sprintf(tempstr, "DELETED");
        color = CV_RGB(0, 0, 0);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &narrowfont, color);       // using narrow font
        // changedid: LIGHT GREEN narrow
        sprintf(tempstr, "CHANGEDID");
        color = CV_RGB(128, 255, 128);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &narrowfont, color);       // using narrow font
        // debug: PURPLE
        sprintf(tempstr, "DEBUG");
        color = CV_RGB(128, 0, 128);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // color2
        tmppoint.y += tmppoint.x;
        // sharesblob: GREEN
        sprintf(tempstr, "SHARESBLOB");
        color = CV_RGB(50, 255, 0);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // sharesblob: BLUE
        sprintf(tempstr, "SHARESID");
        color = CV_RGB(50, 0, 255);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        // sharesblob: LIGHT BLUE
        sprintf(tempstr, "SHARESBLOB & SHARESID");
        color = CV_RGB(50, 255, 255);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);

        // scale bar
        tmppoint.y += tmppoint.x;
        sprintf(tempstr, "100 px");
        color = CV_RGB(255, 255, 255);
        tmppoint.y += tmppoint.x;
        cvPutText(inputimage, tempstr, tmppoint, &font, color);
        tmppoint.y += tmppoint.x / 2;
        cvLine(inputimage, tmppoint, cvPoint(tmppoint.x + 100, tmppoint.y),
                color, 2);
        cvLine(inputimage, cvPoint(tmppoint.x, tmppoint.y - 5),
                cvPoint(tmppoint.x, tmppoint.y + 5), color, 2);
        cvLine(inputimage, cvPoint(tmppoint.x + 100, tmppoint.y - 5),
                cvPoint(tmppoint.x + 100, tmppoint.y + 5), color, 2);

        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvSetImageROI(inputimage, cs->imageROI);
    }
    // OUTPUT_VIDEO_PAIR_ID debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_PAIR_ID) {
        char tempstr[8];

        // plot numbers 1 and 2 to image
        for (tBarcode::iterator itb = mBarcodes.begin();
                itb != mBarcodes.end(); ++itb) {
            if (!(*itb).mFix || ((*itb).mFix & MFIX_DELETED))
                continue;
            if (strcmp((*itb).strid, pair_str[0]) == 0) {
                sprintf(tempstr, "1");
            } else if (strcmp((*itb).strid, pair_str[1]) == 0) {
                sprintf(tempstr, "2");
            } else
                continue;
            tmppoint.x = (int) (*itb).mCenter.x;
            tmppoint.y = (int) (*itb).mCenter.y;
            color = CV_RGB(255, 255, 255);
            cvPutText(inputimage, tempstr, tmppoint, &font, color);
        }
    }
    // write date and time on video anyways
    if (inputvideostarttime) {
        time_t rawtime;
        timed_t rawtimed;
        tm *videotime;
        char tempstr[256];
        // parse filename date or date&time
        rawtimed = inputvideostarttime + currentframe / fps;
        rawtime = (time_t) rawtimed;
        // set hipervideo parameters
        videotime = localtime(&rawtime);
		sprintf(tempstr, COPYRIGHTSTRING "%04d-%02d-%02d %02d:%02d:%02d.%03d #%05d (%s)",
				videotime->tm_year + 1900, videotime->tm_mon + 1, videotime->tm_mday,
				videotime->tm_hour, videotime->tm_min, videotime->tm_sec,
				(int)(1000.0 * (rawtimed - rawtime)), currentframe,
				(mLight == DAYLIGHT ? "DAY" : (mLight == NIGHTLIGHT ? "NIGHT" : \
				(mLight == EXTRALIGHT ? "EXTRA" : "STRANGE"))));       // TODO: add UNINITIALIZED light if needed
        // text starts at top left of ROI image, or original image?
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvResetImageROI(inputimage);
        cvPutText(inputimage, tempstr, cvPoint(2,
                        copyrightTextSize.height + 2), &copyrightfont,
                cvScalar(255, 255, 255));
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvSetImageROI(inputimage, cs->imageROI);
    }
    // write video frame to file
    if (cs->bWriteVideo == 1
            && ((currentframe % cs->outputvideoskipfactor) == 0)) {
        // only save ROI of image
        // TODO bug in OpenCV: videowriter only works with original image size, no ROI.
        if (cs->bApplyROIToVideoOutput) {
            // this will only copy ROI area, and smoothinputimage size is originally framesizeROI so it works (and it is not needed any more)
            cvCopy(inputimage, smoothinputimage);
            cvWriteFrame(videowriter, smoothinputimage);
        }
        // save with original size
        else {
            if (cs->imageROI.width && cs->imageROI.height)
                cvResetImageROI(inputimage);
            // correct image size if needed
            if (bFramesizeMismatch) {
                CvRect temproi;
                temproi.x = temproi.y = 0;
                temproi.width = framesize.width;
                temproi.height = framesize.height;
                cvSetImageROI(outputimage8x, temproi);
                cvCopy(inputimage, outputimage8x);
                cvResetImageROI(outputimage8x);
                cvWriteFrame(videowriter, outputimage8x);
            } else
                cvWriteFrame(videowriter, inputimage);
            if (cs->imageROI.width && cs->imageROI.height)
                cvSetImageROI(inputimage, cs->imageROI);
        }
    }
    // output .jpg screenshot for hiper-speeded video
    if (cs->bWriteVideo
            && ((currentframe % cs->outputscreenshotskipfactor) == 0)) {
        // always save with original size
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvResetImageROI(inputimage);
        char cc[2048];
        sprintf(cc,
                cs->
                bProcessText ? "%s%s" BARCODETAG "_%08d.jpg" :
                "%s%s_%08d.jpg", cs->outputdirectory, cs->outputfilecommon,
                currentframe);
        cvSaveImage(cc, inputimage);
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvSetImageROI(inputimage, cs->imageROI);
    }
    // output .jpg screenshot for Mate's hiper-speeded video
    if (hipervideoskipfactor && cs->bWriteVideo
            && currentframe >= hipervideoframestart
            && currentframe <= hipervideoframeend
            && (currentframe % hipervideoskipfactor) == 0) {
        // always save with original size
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvResetImageROI(inputimage);
        char cc[2048];
        sprintf(cc,
                cs->
                bProcessText ? "%s%s" BARCODETAG "_hiper_%08d.jpg" :
                "%s%s_hiper_%08d.jpg", cs->outputdirectory,
                cs->outputfilecommon, currentframe);
        cvSaveImage(cc, inputimage);
        if (!cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height)
            cvSetImageROI(inputimage, cs->imageROI);
    }
}

////////////////////////////////////////////////////////////////////////////////
// this function sets the hipervideo frame start and end params
bool SetHiperVideoParams(cCS* cs, timed_t inputvideostarttime, double fps) {
	// parse filename date or date&time
	if (inputvideostarttime == 0) {
		LOG_ERROR("File name could not be parsed. Check if format is correct (YYYY-MM-DD_hh-mm-ss).");
		return false;
	}
	// set hipervideo parameters
	time_t tmptime = (time_t)inputvideostarttime;
	tm *videotime = localtime(&tmptime);
	timed_t sec_videotoday =
		videotime->tm_hour * 3600 + videotime->tm_min * 60 +
		videotime->tm_sec + inputvideostarttime - (int)inputvideostarttime;
	timed_t sec_all = cs->hipervideoend - cs->hipervideostart;
	timed_t sec_untilvideo = inputvideostarttime - cs->hipervideostart;
	int day_all = (int)sec_all / 86400;        // 300 days all
	int day_untilvideo = (int)sec_untilvideo / 86400;  /// this is the 25th day --> we need frames at (25/300 --> 26/300)*86400 on this day

	hipervideoframestart = 86400 * day_untilvideo / day_all;    // sec start on this day
	hipervideoframeend = 86400 * (day_untilvideo + 1) / day_all;        // sec end on this day
	hipervideoframestart = (int)((hipervideoframestart - sec_videotoday) * fps);       // frame start on this video (could be out of range)
	hipervideoframeend = (int)((hipervideoframeend - sec_videotoday) * fps);   // frame end on this video (could be out of range)
	hipervideoskipfactor = std::max(1, 86400 / cs->hipervideoduration);       // use every Nth frame between hipervideoframestart and hipervideoframeend

																		// return without error
	return true;
}
