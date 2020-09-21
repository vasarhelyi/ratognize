/* This is the main ratognize application that finds colored blobs on input
 * video streams. Code written by Daniel Abel (abeld@hal.elte.hu) and
 * Gabor Vasarhelyi (vasarhelyi@hal.elte.hu).
 */

#include "barcode.h"
#include "blob.h"
#include "cage.h"
#include "cvutils.h"
#include "datetime.h"
#include "input.h"
#include "log.h"
#include "output_text.h"
#include "output_video.h"
#include "ratognize.h"
#include "version.h"

#ifdef ON_LINUX
#include "libavcodec/avcodec.h" /* for LIBAVCODEC_IDENT version-string */
#endif

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int i;

    // Do all the initialization here
    i = OnInit(argc, argv);
    if (i != 0) {
        OnExit(i > 0);
        return -1 * abs(i);
    }

    // variables for time measurement
    clock_t clock_start = clock(), clock_now;
    double d = 0;

    // log the first frame
    if (cs.bWriteText) {
        ofslog << currentframe << "\tFIRSTFRAME" << std::endl;
    }

    // loop through all frames
    while (!inputimage.empty() && (cs.lastframe < 1 || (cs.lastframe >= 1
                            && currentframe <= cs.lastframe))) {
        // do blob detection and all stuff
        if (!OnStep()) {
            OnExit();
            return -16;
        }
        // calculate framerate, elapsed and remaining time
        clock_now = clock();
        if ((double) (clock_now - clock_start) / CLOCKS_PER_SEC - d >= 1) {
            d = (double) (clock_now - clock_start) / CLOCKS_PER_SEC;
            std::cout << "frame: " << currentframe
                    << ", FPS: " << (double) (currentframe - cs.firstframe) / d
                    << ", elapsed: " << (int) d
                    << "s, remains: " << (int) ((double) d * ((cs.lastframe <
                                    1 ? framecount : cs.lastframe) -
                            currentframe) / (currentframe - cs.firstframe))
                    << "s" << std::endl;
        }
        // read next frame from video
        if (!ReadNextFrame()) {
            break;
        }
    }

    // log the last frame
    if (cs.bWriteText) {
        ofslog << currentframe - 1 << "\tLASTFRAME" << std::endl;
    }

    if (cs.bCout)
        std::cout << std::endl;

    // release memory
    OnExit();
}

////////////////////////////////////////////////////////////////////////////////
int OnInit(int argc, char *argv[]) {
    // some variables
    int i;
    char cc[16];
    bool tempDSLP = false;

#ifdef ON_LINUX
    std::cout << "libavcodec version: " << LIBAVCODEC_IDENT << std::endl;
#endif
    std::cout << "openCV version: " << CV_VERSION << std::endl;
    std::cout << "ratognize version: " << RATOGNIZE_DETAILED_VERSION_STRING << std::endl;

    // check arguments (if these are set, they overwrite .ini settings)
    // no error check, only good ones are
    // TODO: warning, not checked what happens if
    //       spaces are in the file names and it is quoted with ""
    // WARNING: no error check on atoi
    args = argv[0];
    for (int i = 1; i < argc; i++) {
        args.append(" ");
        args.append(argv[i]);
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "--inifile") == 0 && i < argc - 1)
                strncpy(cs.inifile, argv[++i], MAXPATH);
            else if (strcmp(argv[i], "--inputvideofile") == 0 && i < argc - 1)
                strncpy(cs.inputvideofile, argv[++i], MAXPATH);
            else if (strcmp(argv[i], "--dayssincelastpaint") == 0
                    && i < argc - 1) {
                cs.dayssincelastpaint = atoi(argv[++i]);
                tempDSLP = true;
            } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                std::cout << "Usage: ratognize --param1 [filename] --param2 [filename] ..., " << std::endl <<
                        "       where paramN can be 'inifile', 'inputvideofile', 'dayssincelastpaint'" << std::endl <<
                        "       All settings override default and .ini file values." << std::endl;
                return 1;
            } else {
                std::cout << "Unknown option in parameter " << i <<
                        ". Try '--help' if it is not a typo." << std::endl;
                return 2;
            }
        } else                  // odd parameter is not option
        {
            std::cout << "parameter " << i << " is bad. Try '--help'" << std::endl;
            return 3;
        }
    }

    // read control variables
    std::cout << "Reading ini file: " << cs.inifile << std::endl;
    if (!ReadIniFile(tempDSLP, &cs, mColorDataBase, mColor)) {
        std::cin.get();
        return 4;
    }
    std::cout << "  OK - mDiaMin: " << cs.mDiaMin[0] << ".." << cs.mDiaMin[MAXMBASE - 1] <<
            " mDiaMax: " << cs.mDiaMax[0] << ".." << cs.mDiaMax[MAXMBASE - 1] <<
            " mElongationMax: " << cs.mElongationMax[0] << ".." << cs.mElongationMax[MAXMBASE - 1] <<
            std::endl;

    // read paintdate file
    if (cs.paintdatefile[0] && !tempDSLP) {
        std::cout << "Reading paint date file: " << cs.paintdatefile << std::endl;
        if (!ReadPaintDateFile(mPaintDates, &cs)) {
            return 5;
        }
        std::cout << "  OK - number of paint dates: " <<
                (int) mPaintDates.size() << std::endl;
    }

    std::ostringstream outfile;
    // define common output file name part (without path)
    outfile.str(cs.inputvideofile);
    i = (int) outfile.str().find_last_of("\\/");
    strncpy(cs.outputfilecommon, outfile.str().substr(i + 1).c_str(), MAXPATH);
    // correct output directory
    outfile.str(cs.outputdirectory);
    if (*(outfile.str().rbegin()) != '\\' && *(outfile.str().rbegin()) != '/')  // check last char of string
    {
#ifdef ON_LINUX
        cs.outputdirectory[outfile.str().length()] = '/';
#else
        cs.outputdirectory[outfile.str().length()] = '\\';
#endif
        cs.outputdirectory[outfile.str().length() + 1] = 0;
    }
    // and create it if needed
    outfile.str("");
#ifdef ON_LINUX
    outfile << "mkdir -p " << cs.outputdirectory;
#else
    outfile << "mkdir " << cs.outputdirectory;
#endif
    system(outfile.str().c_str());      // TODO: does it work on linux??? \,/ characters, etc.

    // define input/output file names
    std::cout << "Using input video file: " << cs.inputvideofile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".avi" : ".avi");
    strncpy(cs.outputvideofile, outfile.str().c_str(), MAXPATH);  // video
    std::cout << "Using output video file: " << cs.outputvideofile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << ".blobs";
    strncpy(cs.inputdatfile, outfile.str().c_str(), MAXPATH);     // {} style data
    std::cout << "Using input blob file: " << cs.inputdatfile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".blobs" : ".blobs");
    strncpy(cs.outputdatfile, outfile.str().c_str(), MAXPATH);    // {} style data
    std::cout << "Using output blob file: " << cs.outputdatfile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.
            outputfilecommon << (".blobs" BARCODETAG);
    strncpy(cs.inputbarcodefile, outfile.str().c_str(), MAXPATH); // trajognize barcode output file. Watch for common naming convention!
    std::cout << "Using input barcode file: " << cs.inputbarcodefile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << ".log";
    strncpy(cs.inputlogfile, outfile.str().c_str(), MAXPATH);     // used to get LED lines
    std::cout << "Using input log file: " << cs.inputlogfile << std::endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".log" : ".log");
    strncpy(cs.outputlogfile, outfile.str().c_str(), MAXPATH);    // log
    std::cout << "Using output log file: " << cs.outputlogfile << std::endl;

    // set absolute starting time of video
    std::cout << "Get starting date from video file name..." << std::endl;
    i = ParseDateTime(&inputvideostarttime, cs.outputfilecommon);
    // if there are at least two tokens read, we treat it as error,
    // otherwise we assume that date is not coded in the filename
    if (i < -1) {
        std::cout << "  ERROR calling ParseDateTime: " << i << std::endl;
        return 6;
    } else if (i != 6) {
        inputvideostarttime = 0;
    }
    // set dayssincelastpaint
    if (!tempDSLP) {
         if (!SetDSLP(&cs.dayssincelastpaint, inputvideostarttime, mPaintDates)) {
            std::cout << "  ERROR calling SetDSLP. " << std::endl;
            return 7;
        }
    }
    std::cout << "  Input video start time: " << inputvideostarttime << std::endl;
    std::cout << "  Days since last paint: " << cs.dayssincelastpaint << std::endl;

    // Output Fading Colors To File
    if (cs.bShowDebugVideo && cs.colorselectionmethod != 2)
        OutputFadingColorsToFile(&cs, mColorDataBase, mColor, &mBGColor,
                mLight, inputvideostarttime);

    // TODO temp: set nightlight params as default if no LED check is used
    if (!cs.bLED) {
        mLight = NIGHTLIGHT;
        if (!SetHSVDetectionParams(mLight, cs.colorselectionmethod,
                cs.dayssincelastpaint, inputvideostarttime,
                mColorDataBase, mColor, &mBGColor)) {
            return 8;
        }
    }
    // capture video input
    std::cout << "Opening video file..." << std::endl;
    if (!initializeVideo(cs.inputvideofile)) {
        return 9;
    }
    // decrease image size if there is a ROI defined
    // TODO: how should ROI appear in the output coordinates??? What should be the origin?
    if (cs.imageROI.height && cs.imageROI.width) {
        if ((cs.imageROI.height % 8) || (cs.imageROI.width % 8)) {
            std::cout << "  ERROR: imageROI width and height value should be multiples of 8" << std::endl;
            return 10;
        }
        if (framesize.width < cs.imageROI.x + cs.imageROI.width
                || framesize.height < cs.imageROI.y + cs.imageROI.height) {
            std::cout << "  ERROR: imageROI points out of the image frame" << std::endl;
            return 11;
        }
        framesizeROI.width = cs.imageROI.width;
        framesizeROI.height = cs.imageROI.height;
        std::cout << "  image ROI defined, new output size: " << framesizeROI.
                width << "x" << framesizeROI.height << std::endl;
    } else
        framesizeROI = framesize;

    // initialize global images
    smoothinputimage = cv::Mat(framesizeROI, CV_8UC3);    // smooth input image on ROI
    maskimage = cv::Mat(framesizeROI, CV_8UC1);   // binary mask containing rat blobs
    HSVimage = cv::Mat(framesizeROI, CV_8UC3);    // HSV image on ROI - almost all image processing is done on this image

    // get first good frame from video
    if (!readVideoUntilFirstGoodFrame()) {
        return 12;
    }
    // create initial moving average image
    if (cs.bMotionDetection) {
        smoothinputimage.convertTo(movingAverage, CV_32FC3);
    }
    // debug options
    if (cs.bShowVideo) {
        cv::namedWindow("OutputVideo", CV_WINDOW_NORMAL); // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cv::resizeWindow("OutputVideo", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
    }

    if (cs.bShowDebugVideo) {
        // colors
        for (i = 0; i < MAXMBASE; i++) {
            if (!mColor[i].mUse)
                continue;
            snprintf(cc, sizeof(cc), "c%d-%s", i, mColor[i].name);
            cv::namedWindow(cc, CV_WINDOW_NORMAL);        // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
            if (cs.displaywidth)
                cv::resizeWindow(cc, cs.displaywidth,
                        cs.displaywidth * framesizeROI.height /
                        framesizeROI.width);
        }
        // MD
        if (cs.bMotionDetection) {
            cv::namedWindow("MD", CV_WINDOW_NORMAL);
            if (cs.displaywidth)
                cv::resizeWindow("MD", cs.displaywidth,
                        cs.displaywidth * framesizeROI.height /
                        framesizeROI.width);
        }
        // LED
        cv::namedWindow("LED", CV_WINDOW_NORMAL); // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cv::resizeWindow("LED", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
        // rats
        cv::namedWindow("rats", CV_WINDOW_NORMAL);        // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cv::resizeWindow("rats", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
    }
    // init fonts, videowriter, hipervideoparams, etc.
    InitVisualOutput(&cs, framesize, framesizeROI, fps, inputvideostarttime);

    // init input files
    if (cs.bProcessText) {
        if (!OpenInputFileStream(ifsbarcode, cs.inputbarcodefile)) {
            LOG_ERROR("Could not open input barcode file.");
            return -13;
        }
        if (!OpenInputFileStream(ifsdat, cs.inputdatfile)) {
            LOG_ERROR("Could not open input blob file.");
            return -14;
        }
        if (!OpenInputFileStream(ifslog, cs.inputlogfile)) {
            LOG_ERROR("Could not open input log file.");
            return -15;
        }
    } 
    // init output files
    else {
        WriteBlobFileHeader(&cs, ofsdat);
        WriteLogFileHeader(&cs, args, ofslog);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
void OnExit(bool bReleaseVars) {
    if (bReleaseVars) {
        // flush and close files, pipes
        ifsbarcode.close();
        ifsdat.close();
        ifslog.close();
        if (cs.bWriteText) {
            ofslog.flush();
            ofslog.close();
            ofsdat.flush();
            ofsdat.close();
        }
        std::cout.flush();
        std::cerr.flush();
        // release visual outputs
        DestroyVisualOutput();
        // release memory
        if (cs.bShowVideo || cs.bShowDebugVideo) {
            cv::destroyAllWindows();
        }
    }

    if (cs.bCin)
        std::cin.get();
}

////////////////////////////////////////////////////////////////////////////////
bool OnStep() {
    int i;

    // clear particle vectors
    mBlobParticles.clear();
    mMDParticles.clear();
    mRatParticles.clear();
    mBarcodes.clear();

    // run main image processing
    if (cs.bProcessImage) {

        // create images (when it is not initialized or when size changed)
        static cv::Mat filterimage;
        filterimage = cvCreateImageOnce(filterimage, HSVimage.size(), 8, 1, false); // no need to zero

        // detect day/night light from RED LED
        // LED detection is always on on first 50 frames, frame skipping starts only after that
        if (cs.bLED && (currentframe < 50 ||
                (currentframe % cs.LEDdetectionskipfactor) == 0)) {
            // LED detection is on the ORIGINAL frame, not using ROI
            if (!ReadDayNightLED(HSVimage, inputimage, ofslog,
                    &cs, mColorDataBase, mColor, &mBGColor, &mLight,
                    inputvideostarttime, currentframe)) {
                return false;
            }
        }

        // try to detect rats as a whole (and store in global maskimage + as blobs)
        MEASURE_DURATION(DetectRats(HSVimage, maskimage, &mBGColor, &cs,
                mRatParticles, currentframe, ofslog));

        //for (int iii=0;iii<5;iii++) {
        //      std::cout << mColor[iii].name << " ";
        //      for (int jjj=0;jjj<3;jjj++)
        //              std::cout << mColor[iii].mColor.mColorHSV.val[jjj] << " ";
        //      for (int jjj=0;jjj<3;jjj++)
        //              std::cout << mColor[iii].mColor.mRangeHSV.val[jjj] << " ";
        //      std::cout << std::endl;
        //}

        // mask HSVimage with maskimage for main blob detection
        // Note: no mask can be used on sub-calls, so no speed-up is possible
        static cv::Mat maskedHSVimage;
        maskedHSVimage = cvCreateImageOnce(maskedHSVimage, HSVimage.size(),
                IPL_DEPTH_8U, 3);
        HSVimage.copyTo(maskedHSVimage, maskimage);
        // Detect the blobs of all the used colors
        for (i = 0; i < cs.mBase; i++)
            if (mColor[i].mUse) {
                mColor[i].mNumBlobsFound = 0;
                MEASURE_DURATION(FindHSVBlobs(maskedHSVimage, i, filterimage,
                        mColor, &cs, mBlobParticles, currentframe, ofslog));
            }
        //cvReleaseImage(&maskedHSVimage);

        // motion detection filter and MD blobfinder
        if (cs.bMotionDetection) {
            MEASURE_DURATION(FilterMotion(smoothinputimage, movingAverage,
                    filterimage, cs.mdAlpha, cs.mdThreshold));
            if (cs.bShowDebugVideo) {
                cv::imshow("MD", filterimage);
            }
            MEASURE_DURATION(FindMDorRatBlobs(filterimage, &cs, mMDParticles,
                    currentframe, ofslog));
        }
        // Release temporary images
        //cvReleaseImage(&filterimage);
    }

    // load previuosly/externally saved data created by trajognize
    if (cs.bProcessText) {
        // read barcodes from trajognize output
        if (!ReadNextBarcodesFromFile(ifsbarcode, mBarcodes, &cs, currentframe)) {
            return false;
        }
        // read blobs from previous ratognize output
        if (!ReadNextBlobsFromFile(ifsdat, mBlobParticles, mMDParticles,
                mRatParticles, &cs, currentframe)) {
            return false;
        }
        // read log from previous ratognize output (parsing LED lines only)
        i = ReadNextLightFromLogFile(ifslog, &mLight, currentframe);
        if (i < 0) {
            return false;
        } else if (i > 0) {
            if (!SetHSVDetectionParams(mLight, cs.colorselectionmethod,
                    cs.dayssincelastpaint, inputvideostarttime,
                    mColorDataBase, mColor, &mBGColor)) {
                return false;
            }

        }
    }

    // save and show output
    MEASURE_DURATION(GenerateOutput());

    return true;
}

////////////////////////////////////////////////////////////////////////////////
void GenerateOutput() {
    // save data file
    if (cs.bWriteText) {
        WriteBlobFile(&cs, ofsdat, mBlobParticles,
                mMDParticles, mRatParticles, currentframe);
    }

    // write result to output if needed
    if (cs.bCout) {
        std::cout << "frame: " << currentframe
                << ", c0-" << mColor[0].name << ": " << mColor[0].mNumBlobsFound
                << ", c1-" << mColor[1].name << ": " << mColor[1].mNumBlobsFound
                << ", c2-" << mColor[2].name << ": " << mColor[2].mNumBlobsFound
                << ", c3-" << mColor[3].name << ": " << mColor[3].mNumBlobsFound
                << ", c4-" << mColor[4].name << ": " << mColor[4].mNumBlobsFound
                << ", MD: " << (int) mMDParticles.size()
                << ", RAT: " << (int) mRatParticles.size()
                << std::endl;
    }

    //////////////////////////////////////////////////////
    // debug results
    //char* cc = {"RYGBP"};
    //for (int i = 0; i < (int)mBlobParticles.size(); i++) {
    //    std::cout << cc[mBlobParticles[i].index] << mBlobParticles[i].index
    //        << " X" << mBlobParticles[i].mCenter.x
    //        << " Y" << mBlobParticles[i].mCenter.y
    //        << " D" << mBlobParticles[i].mRadius * 2
    //        << " E" << mBlobParticles[i].mAxisA / mBlobParticles[i].mAxisB
    //        << std::endl;
    //}
    ////////////////////////////////////////////////////

    // put blobs on image and show/save it if needed
    if (cs.bShowVideo || cs.bWriteVideo) {
        // generate output video frame
        // pass original image to write to, not ROI one
        WriteVisualOutput(inputimage, &cs,
                mBlobParticles, mMDParticles, mRatParticles,
                mBarcodes, mColor, mLight,
                inputvideostarttime, currentframe, framesize, framesizeROI, fps);
        // show image frame with blobs
        if (cs.bShowVideo) {
            if (cs.bApplyROIToVideoOutput && cs.imageROI.width && cs.imageROI.height) {
                cv::imshow("OutputVideo", inputimage(cs.imageROI));
            } else {
                cv::imshow("OutputVideo", inputimage);
            }

            if (cs.bCin) {
                cvWaitKey(0);   // wait for Return
            } else {
                cvWaitKey(1);   // wait minimal but go on
            }
        }
        // wait for key press anyways
        else if (cs.bShowDebugVideo) {
            if (cs.bCin)
                cvWaitKey(0);   // wait for Return
            else
                cvWaitKey(1);   // wait minimal but go on
        } else if (cs.bCin)
            std::cin.get();
    } else if (cs.bCin) {
        std::cin.get();              // wait for return if needed and no image was shown
    }
}

////////////////////////////////////////////////////////////////////////////////
bool initializeVideo(char *filename) {
    inputvideo.open(filename);

    if (!inputvideo.isOpened()) {
        LOG_ERROR("Could not open input video file: %s", filename);
	return false;
    }
    // set parameters
    framesize.height = (int) inputvideo.get(CV_CAP_PROP_FRAME_HEIGHT);
    if (framesize.height <= 0) {
        LOG_ERROR("Could not get proper framesize.height");
        return false;
    }
    framesize.width = (int) inputvideo.get(CV_CAP_PROP_FRAME_WIDTH);
    if (framesize.width <= 0) {
        LOG_ERROR("Could not get proper framesize.width");
        return false;
    }
    framecount = (int) inputvideo.get(CV_CAP_PROP_FRAME_COUNT);
    if (framecount <= 0) {
        //LOG_ERROR("Could not get proper framecount (received %d)", framecount);
        //return false;

        // TODO: temporary solution, solve this under Linux: bad framecount returned (even negative or zero ???)
        std::cout << "  Warning: could not get proper framecount (received " <<
                framecount << "). Framecount is set to 1000000 temporarily.";
        framecount = 1000000;
        // end of temporary solution
    }
    fps = inputvideo.get(CV_CAP_PROP_FPS);
    // check if input video is interlaced or not
    if (cs.bInputVideoIsInterlaced) {
        fps /= 2;
        framecount /= 2;
        std::cout << "  Warning: input video is manually set as interlaced! Frame number and fps refers to the deinterlaced video." << std::endl;
    } else
        std::cout << "  Warning: Input video is manually set as non-interlaced." <<
                std::endl;
    // output debug info about read variables
    std::cout << "  OK - " << "framecount: " << framecount << ", fps: " << fps <<
            ", framesize: " << framesize.width << "x" << framesize.
            height << std::endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// read until first OK frame
bool readVideoUntilFirstGoodFrame() {
    // TODO:  OpenCv2.3.1 cvSetCaptureProperty does not work well on patek .ts videos!!! Should not be called at all if started from beginning!
    std::cout << "Reading video until first good frame is found..." << std::endl;
    if (cs.firstframe)
        std::cout << "  First frame is defined (" << cs.
                firstframe <<
                "). Since video positioning is not exact, we need to read all frames before that."
                << std::endl;
//      if (cs.firstframe < 100)
//      {
    currentframe = 0;
    while ((inputimage.empty() || currentframe <= cs.firstframe)
            && currentframe < cs.firstframe + 128) {
        ReadNextFrame();
        if ((currentframe % 100) == 0)
            std::cout << currentframe << " frames read (" << cs.firstframe -
                    currentframe << " left)" << std::endl;
    }
//      }
    // TODO: this might not work, do not give large starting frame No. on patek .ts videos!!!
/*	else
    {
        cvSetCaptureProperty(inputvideo,CV_CAP_PROP_POS_FRAMES,currentframe);
        currentframe = cs.firstframe;
        while (!inputimage && currentframe<cs.firstframe + 128)
            ReadNextFrame();
    }
*/
    if (inputimage.empty()) {
        LOG_ERROR("Could not retreive good frame from video even after 128 trials.");
        return false;
    }
    std::cout << "  First good frame number (starting from zero): " <<
            (--currentframe) << std::endl;

    // return without error
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ReadNextFrame() {
    cv::Mat inputimageROI;
    // try to get next frame
    inputvideo.read(inputimage);
    currentframe++;

    // error check
    if (inputimage.empty()) {
        LOG_ERROR("Could not retrieve image from video!");
        return false;
    }
    if (inputimage.channels() != 3) {
        LOG_ERROR("The input image is not a color image.");
        return false;
    }
    // TODO: convert this from c to cpp header style
    //if (memcmp(inputimage->channelSeq, "BGR", 3)) {
    //    LOG_ERROR("The input image is not a BGR image. The result may be unexpected.");
    //    return false;
    //}
    // set image ROI if needed (cvQueryFrame resets it on every call)
    if (cs.imageROI.width && cs.imageROI.height) {
        inputimageROI = inputimage(cs.imageROI);
    } else {
        inputimageROI = inputimage;
    }
    // smooth input image if needed (and possible), but keep original for output video
    if (cs.gausssmoothing) {
        cv::GaussianBlur(inputimageROI, smoothinputimage,
                cv::Size(cs.gausssmoothing, cs.gausssmoothing), 0);
    } else {
        inputimageROI.copyTo(smoothinputimage);
    }
    // convert BGR image to HSV image
    cv::cvtColor(smoothinputimage, HSVimage, CV_BGR2HSV);

    // return without error
    return true;
}
