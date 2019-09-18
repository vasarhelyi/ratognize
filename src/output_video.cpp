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
static cv::VideoWriter videowriter;
static cv::Mat outputimage8x;        // copy of ROI area, the image that will be writte to videowriter
static cv::Size framesize8x;             // if framesize is not multiples of 8, it will be adjusted to save video without error
static bool bFramesizeMismatch = false;
// hipervideo parameters
static int hipervideoframestart;
static int hipervideoframeend;
static int hipervideoskipfactor;
// pair measurement variables
char pair_str[2][8];

void drawtorect(cv::Mat & mat, const std::string & str, cv::Rect target, int face, cv::Scalar color, int thickness) {
    cv::Size rect = cv::getTextSize(str, face, 1.0, thickness, 0);
    double scalex = (double)target.width / (double)rect.width;
    double scaley = (double)target.height / (double)rect.height;
    double scale = std::min(scalex, scaley);
    int marginx = scale == scalex ? 0 : (int)((double)target.width * (scalex - scale) / scalex * 0.5);
    int marginy = scale == scaley ? 0 : (int)((double)target.height * (scaley - scale) / scaley * 0.5);
    cv::putText(mat, str, cv::Point(target.x + marginx, target.y + target.height - marginy), face, scale, color, thickness, 8, false);
}

void InitVisualOutput(cCS* cs, cv::Size framesize, cv::Size framesizeROI,
		double fps, timed_t inputvideostarttime) {
	std::ostringstream outfile;
	int i;

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
		outputimage8x = cv::Mat(framesize8x.height, framesize8x.width, CV_8UC3);    // output image with corrected size
	}

	// init output video with ROI or non-ROI framesize
	if (cs->bWriteVideo) {
#ifdef ON_LINUX
		// use ROI framesize
		if (cs->bApplyROIToVideoOutput) {
			videowriter.open(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesizeROI);
			// use non-ROI framesize
		}
		else {
			videowriter.open(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesize8x);
		}
#else
		// use ROI framesize
		if (cs->bApplyROIToVideoOutput) {
			videowriter.open(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesizeROI);
			//        if (cs->bApplyROIToVideoOutput) videowriter.open(cs->outputvideofile,0,fps,framesizeROI); // no compression
			// use non-ROI framesize
		}
		else {
			videowriter.open(cs->outputvideofile,
				CV_FOURCC('D', 'I', 'V', '3'), fps, framesize8x);
		}
		//              else videowriter.open(cs->outputvideofile,0,fps,framesize8x);  // no compression
#endif
	}
	// set hiper video params
	SetHiperVideoParams(cs, inputvideostarttime, fps);
}

void DestroyVisualOutput() {
	if (videowriter.isOpened()) {
		videowriter.release();
	}
}


void WriteVisualOutput(cv::Mat &inputimage, cv::Mat &smoothinputimage, cCS* cs,
		tBlob& mBlobParticles, tBlob& mMDParticles, tBlob& mRatParticles,
		tBarcode& mBarcodes, cColor* mColor, lighttype_t mLight,
		timed_t inputvideostarttime, int currentframe, cv::Size framesize,
		cv::Size framesizeROI, double fps) {
    //////////////////////////////////////////////////////
    // debug: do not comment out for determining min and max blob sizes
    //cv::putText(inputimage,"dia: cs->mDiaMin, dist: mDistMax",cv::Point(20+2*cs->mDistMax,20),&font,1,cv::Scalar(255,255,255));
    //cvCircle(inputimage,cv::Point(20,20),mDiaMin/2,cv::Scalar(255,255,255),2,CV_AA);
    //cvCircle(inputimage,cv::Point(20+cs->mDistMax,20),cs->mDiaMin/2,cv::Scalar(255,255,255),2,CV_AA);
    //cv::putText(inputimage,"dia: cs->mDiaMax, dist: mDistMax",cv::Point(20+2*cs->mDistMax,20+cs->mDistMax),&font,1,cv::Scalar(255,255,255));
    //cvCircle(inputimage,cv::Point(20,20+cs->mDistMax),cs->mDiaMax/2,cv::Scalar(255,255,255),2,CV_AA);
    //cvCircle(inputimage,cv::Point(20+cs->mDistMax,20+cs->mDistMax),cs->mDiaMax/2,cv::Scalar(255,255,255),2,CV_AA);
    //////////////////////////////////////////////////////

	cv::Mat inputimageROI;
	tBlob::iterator it;
    cv::Point tmppoint;
    cv::Scalar color, color2;
    int dA, dB;
    //cv::Size textSize;
    char cc[3];
    cc[1] = 0;

	// note that we generally write to original image as all coordinates are
	// given for that frame. However, we create a ROI header for convenience

	// OUTPUT_VIDEO_MD debug output:
    if (cs->outputvideotype & OUTPUT_VIDEO_MD) {
        // plot MDParticles to image with black ellipses
        for (it = mMDParticles.begin(); it != mMDParticles.end(); ++it) {
            tmppoint.x = (int) (*it).mCenter.x;
            tmppoint.y = (int) (*it).mCenter.y;
            color = CV_RGB(50, 50, 50);     // DARK GREY
            cv::ellipse(inputimage, tmppoint, cv::Size((int) (*it).mAxisA,
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
            cv::ellipse(inputimage, tmppoint,
                    cv::Size((int)(*it).mAxisA + dA, (int)(*it).mAxisB + dB),
                    Y_IS_DOWN * (*it).mOrientation * 180 / M_PI, 0, 360,
                    color, 2);
            // draw orientation "arrow" as well
            color = CV_RGB(50, 50, 50); // DARKER GRAY
            cv::ellipse(inputimage, tmppoint,
                    cv::Size((int)(*it).mAxisA + dA, (int)(*it).mAxisB + dB),
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
            //cv::ellipse(inputimage,tmppoint,cv::Size((int)(*it).mAxisA,(int)(*it).mAxisB),
            //        Y_IS_DOWN*(*it).mOrientation*180/M_PI,0,360,color,2);
            //cvGetTextSize(cc, &font, &textSize,0);
            //tmppoint.x -= textSize.width/2;
            //tmppoint.y += textSize.height/2;
            color = CV_RGB(255, 255, 255);  // WHITE
            cv::putText(inputimage, cc, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
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
            cv::ellipse(inputimage, tmppoint, cv::Size((int) (*itb).mAxisA,
                            (int) (*itb).mAxisB),
                    Y_IS_DOWN * (*itb).mOrientation * 180 / M_PI, 0, 360,
                    color, (!(*itb).mFix
                            || ((*itb).mFix & MFIX_DELETED)) ? 1 : 2);
            // write barcode string as well
            tmppoint.x += (int) ((*itb).mAxisA * cos((*itb).mOrientation));
            tmppoint.y +=
                    (int) ((*itb).mAxisA * sin(Y_IS_DOWN *
                            (*itb).mOrientation));
            cv::putText(inputimage, (*itb).strid, tmppoint,
					CV_FONT_HERSHEY_SIMPLEX, 0.8, color2,
					(!(*itb).mFix || ((*itb).mFix & MFIX_DELETED)) ? 1 : 2);

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
                        snprintf(tempstr, sizeof(tempstr), ",%d", vel);
                        tmppoint.x += 50;
                        cv::putText(inputimage, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
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
        if (cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height) {
			inputimageROI = inputimage(cs->imageROI);
		} else {
			inputimageROI = inputimage;
		}

        char tempstr[256];
        tmppoint.x = 25;
        tmppoint.y = 40;
        // color
        // fullfound: RED
        snprintf(tempstr, sizeof(tempstr), "FULLFOUND");
        color = CV_RGB(255, 0, 0);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // fullnocluster: DARK RED
        snprintf(tempstr, sizeof(tempstr), "FULLNOCLUSTER");
        color = CV_RGB(128, 0, 0);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // partlyfound: PINK
        snprintf(tempstr, sizeof(tempstr), "PARTLYFOUND_FROM_TDIST");
        color = CV_RGB(255, 128, 128);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // chosen: WHITE
        snprintf(tempstr, sizeof(tempstr), "CHOSEN");
        color = CV_RGB(255, 255, 255);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // virtual+chosen: YELLOW
        snprintf(tempstr, sizeof(tempstr), "VIRTUAL");
        color = CV_RGB(255, 255, 0);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // deleted: BLACK narrow
        snprintf(tempstr, sizeof(tempstr), "DELETED");
        color = CV_RGB(0, 0, 0);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 1);       // using narrow font
        // changedid: LIGHT GREEN narrow
        snprintf(tempstr, sizeof(tempstr), "CHANGEDID");
        color = CV_RGB(128, 255, 128);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_VECTOR0, 0.8, color, 1);       // using narrow font
        // debug: PURPLE
        snprintf(tempstr, sizeof(tempstr), "DEBUG");
        color = CV_RGB(128, 0, 128);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // color2
        tmppoint.y += tmppoint.x;
        // sharesblob: GREEN
        snprintf(tempstr, sizeof(tempstr), "SHARESBLOB");
        color = CV_RGB(50, 255, 0);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // sharesblob: BLUE
        snprintf(tempstr, sizeof(tempstr), "SHARESID");
        color = CV_RGB(50, 0, 255);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        // sharesblob: LIGHT BLUE
        snprintf(tempstr, sizeof(tempstr), "SHARESBLOB & SHARESID");
        color = CV_RGB(50, 255, 255);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);

        // scale bar
        tmppoint.y += tmppoint.x;
        snprintf(tempstr, sizeof(tempstr), "100 px");
        color = CV_RGB(255, 255, 255);
        tmppoint.y += tmppoint.x;
        cv::putText(inputimageROI, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
        tmppoint.y += tmppoint.x / 2;
        cv::line(inputimageROI, tmppoint, cv::Point(tmppoint.x + 100, tmppoint.y),
                color, 2);
        cv::line(inputimageROI, cv::Point(tmppoint.x, tmppoint.y - 5),
                cv::Point(tmppoint.x, tmppoint.y + 5), color, 2);
        cv::line(inputimageROI, cv::Point(tmppoint.x + 100, tmppoint.y - 5),
                cv::Point(tmppoint.x + 100, tmppoint.y + 5), color, 2);
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
                snprintf(tempstr, sizeof(tempstr), "1");
            } else if (strcmp((*itb).strid, pair_str[1]) == 0) {
                snprintf(tempstr, sizeof(tempstr), "2");
            } else
                continue;
            tmppoint.x = (int) (*itb).mCenter.x;
            tmppoint.y = (int) (*itb).mCenter.y;
            color = CV_RGB(255, 255, 255);
			// TODO: output to ROI or original image?
			cv::putText(inputimage, tempstr, tmppoint, CV_FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
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
		snprintf(tempstr, sizeof(tempstr), COPYRIGHTSTRING "%04d-%02d-%02d %02d:%02d:%02d.%03d #%05d (%s)",
				videotime->tm_year + 1900, videotime->tm_mon + 1, videotime->tm_mday,
				videotime->tm_hour, videotime->tm_min, videotime->tm_sec,
				(int)(1000.0 * (rawtimed - rawtime)), currentframe,
				(mLight == DAYLIGHT ? "DAY" : (mLight == NIGHTLIGHT ? "NIGHT" : \
				(mLight == EXTRALIGHT ? "EXTRA" : "STRANGE"))));       // TODO: add UNINITIALIZED light if needed
        // text starts at top left of ROI image, or original image?
        if (cs->bApplyROIToVideoOutput && cs->imageROI.width
                && cs->imageROI.height) {
			inputimageROI = inputimage(cs->imageROI);
		} else {
			inputimageROI = inputimage;
		}
    	drawtorect(inputimageROI, tempstr,
				cv::Rect(2, 2, framesizeROI.width, framesizeROI.height / 10),
				CV_FONT_HERSHEY_SIMPLEX, cv::Scalar(255, 255, 255), 2);

    }
    // write video frame to file
    if (cs->bWriteVideo == 1
            && ((currentframe % cs->outputvideoskipfactor) == 0)) {
        // only save ROI of image
        if (cs->bApplyROIToVideoOutput) {
            videowriter.write(inputimage(cs->imageROI));
        }
        // save with original size
        else {
            // correct image size if needed
            if (bFramesizeMismatch) {
                CvRect temproi;
                temproi.x = temproi.y = 0;
                temproi.width = framesize.width;
                temproi.height = framesize.height;
                outputimage8x = inputimage(temproi);
                videowriter.write(outputimage8x);
            } else
                videowriter.write(inputimage);
        }
    }
    // output .jpg screenshot for hiper-speeded video
    if (cs->bWriteVideo && ((currentframe % cs->outputscreenshotskipfactor) == 0)) {
        char cc[2048];
        snprintf(cc, sizeof(cc),
                cs->
                bProcessText ? "%s%s" BARCODETAG "_%08d.jpg" :
                "%s%s_%08d.jpg", cs->outputdirectory, cs->outputfilecommon,
                currentframe);
        cv::imwrite(cc, inputimage);
    }
    // output .jpg screenshot for Mate's hiper-speeded video
    if (hipervideoskipfactor && cs->bWriteVideo
            && currentframe >= hipervideoframestart
            && currentframe <= hipervideoframeend
            && (currentframe % hipervideoskipfactor) == 0) {
        char cc[2048];
        snprintf(cc, sizeof(cc),
                cs->
                bProcessText ? "%s%s" BARCODETAG "_hiper_%08d.jpg" :
                "%s%s_hiper_%08d.jpg", cs->outputdirectory,
                cs->outputfilecommon, currentframe);
        cv::imwrite(cc, inputimage);
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
