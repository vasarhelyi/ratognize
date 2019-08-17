#include "cvutils.h"

IplImage *cvCreateImageOnce(IplImage * dst, CvSize size, int depth,
        int channels, bool bZero) {
    // create image if not null
    if (!dst)
        dst = cvCreateImage(size, depth, channels);
    // if size mismatch, recreate it
    else if (dst->width != size.width || dst->height != size.height) {
        cvReleaseImage(&dst);
        dst = cvCreateImage(size, depth, channels);
    }
    // if already created, zero it
    if (bZero)
        cvZero(dst);

    // return image
    return dst;
}

void cvFilterHSV(IplImage * dstBin, IplImage * srcHSV, CvScalar * colorHSV,
        CvScalar * rangeHSV) {
    // create black binary image as dst
    cvSetZero(dstBin);

    uchar *temp;
    int Hmin, Hmax, Smin, Smax, Vmin, Vmax, x;

    // Hue: 0-180, circular continuous
    Hmax = (int) colorHSV->val[0];
    Hmin = Hmax;
    x = (int) rangeHSV->val[0];
    if (x > 89)
        x = 89;
    Hmax = (Hmax + x) % 180;
    Hmin = (Hmin + 180 - x) % 180;

    // Saturation: 0-255
    Smax = (int) colorHSV->val[1];
    Smin = Smax;
    x = (int) rangeHSV->val[1];
    Smax += x;
    if (Smax > 255)
        Smax = 255;
    Smin -= x;
    if (Smin < 0)
        Smin = 0;

    // Value: 0-255
    Vmax = (int) colorHSV->val[2];
    Vmin = Vmax;
    x = (int) rangeHSV->val[2];
    Vmax += x;
    if (Vmax > 255)
        Vmax = 255;
    Vmin -= x;
    if (Vmin < 0)
        Vmin = 0;

    // set nonzero pixels only
    for (int i = 0; i < srcHSV->width; i++) {
        for (int j = 0; j < srcHSV->height; j++) {
            temp = &((uchar *) (srcHSV->imageData +
                            srcHSV->widthStep * j))[i * 3];
            if (((Hmax >= Hmin && temp[0] >= Hmin && temp[0] <= Hmax)
                            || (Hmax < Hmin && (temp[0] > Hmin
                                            || temp[0] < Hmax)))
                    && temp[1] >= Smin && temp[1] <= Smax && temp[2] >= Vmin
                    && temp[2] <= Vmax) {
                ((uchar *) (dstBin->imageData + dstBin->widthStep * j))[i] =
                        255;
            }
        }
    }
}

void cvFilterHSV2(IplImage * dstBin, IplImage * srcHSV, CvScalar * colorHSV,
        CvScalar * rangeHSV) {
    int Hmin, Hmax, Smin, Smax, Vmin, Vmax, x;

    // Hue: 0-180, circular continuous
    Hmin = Hmax = (int) colorHSV->val[0];
    x = (int) rangeHSV->val[0];
    if (x > 89)
        x = 89;
    Hmax = (Hmax + x) % 180;
    Hmin = (Hmin + 180 - x) % 180;

    // Saturation: 0-255
    Smin = Smax = (int) colorHSV->val[1];
    x = (int) rangeHSV->val[1];
    Smax += x;
    if (Smax > 255)
        Smax = 255;
    Smin -= x;
    if (Smin < 0)
        Smin = 0;

    // Value: 0-255
    Vmin = Vmax = (int) colorHSV->val[2];
    x = (int) rangeHSV->val[2];
    Vmax += x;
    if (Vmax > 255)
        Vmax = 255;
    Vmin -= x;
    if (Vmin < 0)
        Vmin = 0;

    // threshold H plane
    if (Hmax >= Hmin) {
        cvInRangeS(srcHSV, cvScalar(Hmin, Smin, Vmin),
                cvScalar(Hmax, Smax, Vmax), dstBin);
    } else {
        static IplImage *tmp = NULL;
        tmp = cvCreateImageOnce(tmp, cvGetSize(dstBin), 8, 1, false);
        cvInRangeS(srcHSV, cvScalar(Hmin, Smin, Vmin),
                cvScalar(255, Smax, Vmax), dstBin);
        cvInRangeS(srcHSV, cvScalar(0, Smin, Vmin),
                cvScalar(Hmax, Smax, Vmax), tmp);
        cvOr(tmp, dstBin, dstBin);
        //cvReleaseImage(&tmp);
    }
}

void cvSkeleton(CvArr * src, CvArr * dst) {
    static IplImage *temp = NULL;
    static IplImage *eroded = NULL;
    static IplImage *skel = NULL;
    IplConvKernel *element;

    temp = cvCreateImageOnce(temp, cvGetSize(src), IPL_DEPTH_8U, 1, false);
    eroded = cvCreateImageOnce(eroded, cvGetSize(src), IPL_DEPTH_8U, 1, false);
    skel = cvCreateImageOnce(skel, cvGetSize(src), IPL_DEPTH_8U, 1);
    element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_CROSS);

    cvCopy(src, dst);

    do {
        cvErode(dst, eroded, element);
        cvDilate(eroded, temp, element);
        cvSub(dst, temp, temp);
        cvOr(skel, temp, skel);
        cvCopy(eroded, dst);
    } while (cvNorm(dst));

    cvCopy(skel, dst);

    //cvReleaseImage(&temp);
    //cvReleaseImage(&eroded);
    //cvReleaseImage(&skel);
}

void FilterMotion(IplImage * srcColor, IplImage * movingAverage,
        IplImage * dstGrey, double mdAlpha, int mdThreshold) {
    //Images to use in the program.
    static IplImage *difference = NULL;
    static IplImage *dred = NULL;
    static IplImage *dgreen = NULL;
    static IplImage *dblue = NULL;

    // create diff image
    difference = cvCreateImageOnce(difference, cvGetSize(srcColor), 
			IPL_DEPTH_8U, 3, false);

    //Convert the scale of the moving average and store in difference.
    cvConvert(movingAverage, difference);

    //Substract the current frame from the moving average.
    cvAbsDiff(srcColor, difference, difference);

    // update running average with current frame
    cvRunningAvg(srcColor, movingAverage, mdAlpha, NULL);

    // add channels in difference image
    dred = cvCreateImageOnce(dred, cvGetSize(difference), IPL_DEPTH_8U, 1,
            false);
    dgreen = cvCreateImageOnce(dgreen, cvGetSize(difference), IPL_DEPTH_8U, 1,
            false);
    dblue = cvCreateImageOnce(dblue, cvGetSize(difference), IPL_DEPTH_8U, 1,
            false);
    cvSplit(difference, dblue, dgreen, dred, NULL);

	// add individual channel differences
	cvAdd(dblue, dgreen, dstGrey);
	cvAdd(dred, dstGrey, dstGrey);
	// or choose max deviation
	// (this should be used for PROJECT_MAZE)
	// cvMax(dblue, dgreen, dstGrey);
    // cvMax(dred, dstGrey, dstGrey);

    // debug output: average difference on new image
    //CvScalar avgdiff = cvAvg(dstGrey);
    //ofslog << currentframe << "\tAVGDIFF\t" << avgdiff.val[0] << endl;

    //Convert the image to grayscale.
    //cvCvtColor(difference,dstGrey,CV_RGB2GRAY);

    //Convert the image to black and white.
    cvThreshold(dstGrey, dstGrey, mdThreshold, 255, CV_THRESH_BINARY);

    //Dilate and erode to get moving blobs
    //TODO: these parameters can be optimized, too/
    cvDilate(dstGrey, dstGrey, 0, 6);
    cvErode(dstGrey, dstGrey, 0, 4);

    // Destroy the image, movies, and window.
    //cvReleaseImage(&difference);
    //cvReleaseImage(&dred);
    //cvReleaseImage(&dgreen);
    //cvReleaseImage(&dblue);
}

void FitLine(CvPoint * points, int count, float *line) {
    double x = 0, y = 0, xy = 0, xx = 0;
    for (int i = 0; i < count; i++) {
        x += points[i].x;
        y += points[i].y;
        xy += points[i].x * points[i].y;
        xx += points[i].x * points[i].x;
    }
    // slope
    line[1] = (float) ((count * xy - x * y) / (count * xx - x * x));    // TODO: no error check on vertical line
    // intercept
    line[0] = (float) ((y - line[1] * x) / count);
}
