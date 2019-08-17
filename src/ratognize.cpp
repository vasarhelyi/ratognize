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

#ifdef ON_LINUX
#include "libavcodec/avcodec.h" /* for LIBAVCODEC_IDENT version-string */
#endif

#define RATOGNIZE__VERSION "$Id: ratognize.cpp 10039 2019-01-24 07:56:04Z vasarhelyi $"

using namespace std;

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
    ofslog << currentframe << "\tFIRSTFRAME" << endl;

    // loop through all frames
    while (inputimage && (cs.lastframe < 1 || (cs.lastframe >= 1
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
            cout << "frame: " << currentframe
                    << ", FPS: " << (double) (currentframe - cs.firstframe) / d
                    << ", elapsed: " << (int) d
                    << "s, remains: " << (int) ((double) d * ((cs.lastframe <
                                    1 ? framecount : cs.lastframe) -
                            currentframe) / (currentframe - cs.firstframe))
                    << "s" << endl;
        }
        // read next frame from video
        if (!ReadNextFrame()) {
            break;
        }
    }

    // log the last frame
    ofslog << currentframe - 1 << "\tLASTFRAME" << endl;

    if (cs.bCout)
        cout << endl;

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
    cout << "libavcodec version: " << LIBAVCODEC_IDENT << endl;
#endif
    cout << "openCV version: " << CV_VERSION << endl;
    cout << "ratognize.h version: " << RATOGNIZE_H__VERSION << endl;
    cout << "ratognize.cpp version: " << RATOGNIZE__VERSION << endl;

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
            if (strcmp(argv[i], "-inifile") == 0 && i < argc - 1)
                strcpy(cs.inifile, argv[++i]);
            else if (strcmp(argv[i], "-inputvideofile") == 0 && i < argc - 1)
                strcpy(cs.inputvideofile, argv[++i]);
            else if (strcmp(argv[i], "-dayssincelastpaint") == 0
                    && i < argc - 1) {
                cs.dayssincelastpaint = atoi(argv[++i]);
                tempDSLP = true;
            } else if (strcmp(argv[i], "-help") == 0) {
                cout << "Usage: ratognize -param1 [filename] -param2 [filename] ..., " << endl << 
                        "       where paramN can be 'inifile', 'inputvideofile', 'dayssincelastpaint'" << endl << 
                        "       All settings override default and .ini file values." << endl;
                return 1;
            } else {
                cout << "Unknown option in parameter " << i <<
                        ". Try '-help' if it is not a typo." << endl;
                return 2;
            }
        } else                  // odd parameter is not option
        {
            cout << "parameter " << i << " is bad. Try '-help'" << endl;
            return 3;
        }
    }

    // read control variables
    cout << "Reading ini file: " << cs.inifile << endl;
    if (!ReadIniFile(tempDSLP, &cs, mColorDataBase, mColor)) {
        cin.get();
        return 4;
    }
    cout << "  OK - mDiaMin: " << cs.mDiaMin << " mDiaMax: " << cs.mDiaMax <<  
            " mElongationMax: " << cs.mElongationMax << endl;

    // read paintdate file
    if (cs.paintdatefile[0] && !tempDSLP) {
        cout << "Reading paint date file: " << cs.paintdatefile << endl;
        if (!ReadPaintDateFile(mPaintDates, &cs)) {
            return 5;
        }
        cout << "  OK - number of paint dates: " << 
                (int) mPaintDates.size() << endl;
    }

    std::ostringstream outfile;
    // define common output file name part (without path)
    outfile.str(cs.inputvideofile);
    i = (int) outfile.str().find_last_of("\\/");
    strcpy(cs.outputfilecommon, outfile.str().substr(i + 1).c_str());
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
    outfile << "mkdir " << cs.outputdirectory;
    system(outfile.str().c_str());      // TODO: does it work on linux??? \,/ characters, etc.

    // define input/output file names
    cout << "Using input video file: " << cs.inputvideofile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".avi" : ".avi");
    strcpy(cs.outputvideofile, outfile.str().c_str());  // video
    cout << "Using output video file: " << cs.outputvideofile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << ".blobs";
    strcpy(cs.inputdatfile, outfile.str().c_str());     // {} style data
    cout << "Using input blob file: " << cs.inputdatfile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".blobs" : ".blobs");
    strcpy(cs.outputdatfile, outfile.str().c_str());    // {} style data
    cout << "Using output blob file: " << cs.outputdatfile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.
            outputfilecommon << (".blobs" BARCODETAG);
    strcpy(cs.inputbarcodefile, outfile.str().c_str()); // trajognize barcode output file. Watch for common naming convention!
    cout << "Using input barcode file: " << cs.inputbarcodefile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << ".log";
    strcpy(cs.inputlogfile, outfile.str().c_str());     // used to get LED lines
    cout << "Using input log file: " << cs.inputlogfile << endl;

    outfile.str("");
    outfile << cs.outputdirectory << cs.outputfilecommon << (cs.
            bProcessText ? BARCODETAG ".log" : ".log");
    strcpy(cs.outputlogfile, outfile.str().c_str());    // log
    cout << "Using output log file: " << cs.outputlogfile << endl;

    // set absolute starting time of video
    cout << "Get starting date from video file name..." << endl;
    i = ParseDateTime(&inputvideostarttime, cs.outputfilecommon);
    // if there are at least two tokens read, we treat it as error,
    // otherwise we assume that date is not coded in the filename
    if (i < -1) {
        cout << "  ERROR calling ParseDateTime: " << i << endl;
        return 6;
    } else if (i != 6) {
        inputvideostarttime = 0;
    }
    // set dayssincelastpaint
    if (!tempDSLP) {
         if (!SetDSLP(&cs.dayssincelastpaint, inputvideostarttime, mPaintDates)) {
            cout << "  ERROR calling SetDSLP. " << endl;
            return 7;
        }
    }
    cout << "  Input video start time: " << inputvideostarttime << endl;
    cout << "  Days since last paint: " << cs.dayssincelastpaint << endl;

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
    cout << "Opening video file..." << endl;
    if (!initializeVideo(cs.inputvideofile)) {
        return 9;
    }
    // decrease image size if there is a ROI defined
    // TODO: how should ROI appear in the output coordinates??? What should be the origin?
    if (cs.imageROI.height && cs.imageROI.width) {
        if ((cs.imageROI.height % 8) || (cs.imageROI.width % 8)) {
            cout << "  ERROR: imageROI width and height value should be multiples of 8" << endl;
            return 10;
        }
        if (framesize.width < cs.imageROI.x + cs.imageROI.width
                || framesize.height < cs.imageROI.y + cs.imageROI.height) {
            cout << "  ERROR: imageROI points out of the image frame" << endl;
            return 11;
        }
        framesizeROI.width = cs.imageROI.width;
        framesizeROI.height = cs.imageROI.height;
        cout << "  image ROI defined, new output size: " << framesizeROI.
                width << "x" << framesizeROI.height << endl;
    } else
        framesizeROI = framesize;

    // initialize global images
    smoothinputimage = cvCreateImage(framesizeROI, IPL_DEPTH_8U, 3);    // smooth input image on ROI
    maskimage = cvCreateImage(framesizeROI, IPL_DEPTH_8U, 1);   // binary mask containing rat blobs
    HSVimage = cvCreateImage(framesizeROI, IPL_DEPTH_8U, 3);    // HSV image on ROI - almost all image processing is done on this image

    // get first good frame from video
    if (!readVideoUntilFirstGoodFrame()) {
        return 12;
    }
    // create initial moving average image
    if (cs.bMotionDetection) {
        movingAverage = cvCreateImage(framesizeROI, IPL_DEPTH_32F, 3);  // high resolution depth
        cvConvert(smoothinputimage, movingAverage);
    }
    // debug options
    if (cs.bShowVideo) {
        cvNamedWindow("OutputVideo", CV_WINDOW_NORMAL); // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cvResizeWindow("OutputVideo", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
    }

    if (cs.bShowDebugVideo) {
        // colors
        for (i = 0; i < MAXMBASE; i++) {
            if (!mColor[i].mUse)
                continue;
            sprintf(cc, "c%d-%s", i, mColor[i].name);
            cvNamedWindow(cc, CV_WINDOW_NORMAL);        // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
            if (cs.displaywidth)
                cvResizeWindow(cc, cs.displaywidth,
                        cs.displaywidth * framesizeROI.height /
                        framesizeROI.width);
        }
        // MD
        if (cs.bMotionDetection) {
            cvNamedWindow("MD", CV_WINDOW_NORMAL);
            if (cs.displaywidth)
                cvResizeWindow("MD", cs.displaywidth,
                        cs.displaywidth * framesizeROI.height /
                        framesizeROI.width);
        }
        // LED
        cvNamedWindow("LED", CV_WINDOW_NORMAL); // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cvResizeWindow("LED", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
        // rats
        cvNamedWindow("rats", CV_WINDOW_NORMAL);        // | CV_GUI_EXPANDED | CV_WINDOW_KEEPRATIO);
        if (cs.displaywidth)
            cvResizeWindow("rats", cs.displaywidth,
                    cs.displaywidth * framesizeROI.height / framesizeROI.width);
    }
    // init fonts, videowriter, hipervideoparams, etc.
    InitVisualOutput(&cs, framesize, framesizeROI, fps, inputvideostarttime);

    // init output files
    WriteBlobFileHeader(&cs, ofsdat);
    WriteLogFileHeader(&cs, args, ofslog);
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

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
void OnExit(bool bReleaseVars) {
    if (bReleaseVars) {
        // flush and close files, pipes
        ifsbarcode.close();
        ifsdat.close();
        ifslog.close();
        ofslog.flush();
        ofslog.close();
        ofsdat.flush();
        ofsdat.close();
        cout.flush();
        cerr.flush();
        // release visual outputs
        DestroyVisualOutput();
        // release memory
        if (cs.bMotionDetection) {
            cvReleaseImage(&movingAverage);
        }
        if (cs.bShowVideo || cs.bShowDebugVideo) {
            cvDestroyAllWindows();
        }
        cvReleaseImage(&smoothinputimage);
        cvReleaseImage(&maskimage);
        cvReleaseImage(&HSVimage);
    }

    if (cs.bCin)
        cin.get();
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
        static IplImage *filterimage = NULL;
        filterimage = cvCreateImageOnce(filterimage, cvGetSize(HSVimage), 8, 1, false); // no need to zero

        // detect day/night light from RED LED
        // LED detection is always on on first 50 frames, frame skipping starts only after that
        if (cs.bLED && (currentframe < 50 || 
                (currentframe % cs.LEDdetectionskipfactor) == 0)) {
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
        //      cout << mColor[iii].name << " ";
        //      for (int jjj=0;jjj<3;jjj++) 
        //              cout << mColor[iii].mColor.mColorHSV.val[jjj] << " ";
        //      for (int jjj=0;jjj<3;jjj++) 
        //              cout << mColor[iii].mColor.mRangeHSV.val[jjj] << " ";
        //      cout << endl;
        //}

        // mask HSVimage with maskimage for main blob detection
        // Note: no mask can be used on sub-calls, so no speed-up is possible
        static IplImage *maskedHSVimage = NULL;
        maskedHSVimage = cvCreateImageOnce(maskedHSVimage, cvGetSize(HSVimage),
                IPL_DEPTH_8U, 3);
        cvCopy(HSVimage, maskedHSVimage, maskimage);
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
                cvShowImage("MD", filterimage);
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
        cout << "frame: " << currentframe
                << ", c0-" << mColor[0].name << ": " << mColor[0].mNumBlobsFound
                << ", c1-" << mColor[1].name << ": " << mColor[1].mNumBlobsFound
                << ", c2-" << mColor[2].name << ": " << mColor[2].mNumBlobsFound
                << ", c3-" << mColor[3].name << ": " << mColor[3].mNumBlobsFound
                << ", c4-" << mColor[4].name << ": " << mColor[4].mNumBlobsFound
                << ", MD: " << (int) mMDParticles.size()
                << ", RAT: " << (int) mRatParticles.size()
                << endl;
    }

    //////////////////////////////////////////////////////
    // debug results
    //char* cc = {"RYGBP"};
    //for (int i = 0; i < (int)mBlobParticles.size(); i++) {
    //    cout << cc[mBlobParticles[i].index] << mBlobParticles[i].index
    //        << " X" << mBlobParticles[i].mCenter.x
    //        << " Y" << mBlobParticles[i].mCenter.y
    //        << " D" << mBlobParticles[i].mRadius * 2
    //        << " E" << mBlobParticles[i].mAxisA / mBlobParticles[i].mAxisB
    //        << endl;
    //}
    ////////////////////////////////////////////////////

    // put blobs on image and show/save it if needed
    if (cs.bShowVideo || cs.bWriteVideo) {
        // generate output video frame
        WriteVisualOutput(inputimage, smoothinputimage, &cs,
                mBlobParticles, mMDParticles, mRatParticles,
                mBarcodes, mColor, mLight,
                inputvideostarttime, currentframe, framesize, fps);
        // show image with blobs
        if (cs.bShowVideo) {
            cvShowImage("OutputVideo", inputimage);     // show frame
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
            cin.get();
    } else if (cs.bCin) {
        cin.get();              // wait for return if needed and no image was shown
    }
}

////////////////////////////////////////////////////////////////////////////////
bool initializeVideo(char *filename) {
    inputvideo = cvCaptureFromAVI(filename);    // TODO: OpenCv2.3.1 does not work on X64 Debug/Release

    if (!inputvideo) {
        LOG_ERROR("Could not open input video file: %s", filename);
    }
    // set parameters
    framesize.height = (int) cvGetCaptureProperty(inputvideo,
            CV_CAP_PROP_FRAME_HEIGHT);
    if (framesize.height <= 0) {
        LOG_ERROR("Could not get proper framesize.height");
        return false;
    }
    framesize.width = (int) cvGetCaptureProperty(inputvideo,
            CV_CAP_PROP_FRAME_WIDTH);
    if (framesize.width <= 0) {
        LOG_ERROR("Could not get proper framesize.width");
        return false;
    }
    framecount = (int) cvGetCaptureProperty(inputvideo,
            CV_CAP_PROP_FRAME_COUNT);
    if (framecount <= 0) {
        //LOG_ERROR("Could not get proper framecount (received %d)", framecount);
        //return false;

        // TODO: temporary solution, solve this under Linux: bad framecount returned (even negative or zero ???)
        cout << "  Warning: could not get proper framecount (received " <<
                framecount << "). Framecount is set to 1000000 temporarily.";
        framecount = 1000000;
        // end of temporary solution
    }
    fps = cvGetCaptureProperty(inputvideo, CV_CAP_PROP_FPS);
    // check if input video is interlaced or not
    if (cs.bInputVideoIsInterlaced) {
        fps /= 2;
        framecount /= 2;
        cout << "  Warning: input video is manually set as interlaced! Frame number and fps refers to the deinterlaced video." << endl;
    } else
        cout << "  Warning: Input video is manually set as non-interlaced." <<
                endl;
    // output debug info about read variables
    cout << "  OK - " << "framecount: " << framecount << ", fps: " << fps <<
            ", framesize: " << framesize.width << "x" << framesize.
            height << endl;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// read until first OK frame
bool readVideoUntilFirstGoodFrame() {
    // TODO:  OpenCv2.3.1 cvSetCaptureProperty does not work well on patek .ts videos!!! Should not be called at all if started from beginning!
    cout << "Reading video until first good frame is found..." << endl;
    if (cs.firstframe)
        cout << "  First frame is defined (" << cs.
                firstframe <<
                "). Since video positioning is not exact, we need to read all frames before that."
                << endl;
//      if (cs.firstframe < 100)
//      {
    currentframe = 0;
    while ((!inputimage || currentframe <= cs.firstframe)
            && currentframe < cs.firstframe + 128) {
        ReadNextFrame();
        if ((currentframe % 100) == 0)
            cout << currentframe << " frames read (" << cs.firstframe -
                    currentframe << " left)" << endl;
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
    if (!inputimage) {
        LOG_ERROR("Could not retreive good frame from video even after 128 trials.");
        return false;
    }
    cout << "  First good frame number (starting from zero): " <<
            (--currentframe) << endl;

    // return without error
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ReadNextFrame() {
    // try to get next frame
    inputimage = cvQueryFrame(inputvideo);
    currentframe++;

    // error check
    if (!inputimage) {
        LOG_ERROR("Could not retrieve image from video!");
        return false;
    }
    if (inputimage->nChannels != 3) {
        LOG_ERROR("The input image is not a color image.");
        return false;
    }
    if (memcmp(inputimage->channelSeq, "BGR", 3)) {
        LOG_ERROR("The input image is not a BGR image. The result may be unexpected.");
        return false;
    }
    // set image ROI if needed (cvQueryFrame resets it on every call)
    if (cs.imageROI.width && cs.imageROI.height) {
        cvSetImageROI(inputimage, cs.imageROI);
    }
    // smooth input image if needed (and possible), but keep original for output video
    if (cs.gausssmoothing) {
        cvSmooth(inputimage, smoothinputimage, CV_GAUSSIAN, cs.gausssmoothing);
    } else {
        cvCopy(inputimage, smoothinputimage);
    }
    // convert BGR image to HSV image
    cvCvtColor(smoothinputimage, HSVimage, CV_BGR2HSV);

    // return without error
    return true;
}
