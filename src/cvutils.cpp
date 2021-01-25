#include "cvutils.h"

cv::Mat cvCreateImageOnce(cv::Mat &dst, cv::Size size, int depth,
        int channels, bool bZero) {
    // create image if not empty
    if (dst.empty())
        dst = cv::Mat(size, depth, channels);
    // if size mismatch, recreate it
    else if (dst.size().width != size.width || dst.size().height != size.height) {
        dst = cv::Mat(size, depth, channels);
    }
    // if already created, zero it
    if (bZero)
        dst.setTo(cv::Scalar::all(0));

    // return image
    return dst;
}

void cvFilterHSV(cv::Mat &dstBin, cv::Mat &srcHSV, cv::Scalar colorHSV,
        cv::Scalar rangeHSV) {
    int Hmin, Hmax, Smin, Smax, Vmin, Vmax, x;

    // Hue: 0-180, circular continuous
    Hmin = Hmax = (int) colorHSV.val[0];
    x = (int) rangeHSV.val[0];
    if (x > 89)
        x = 89;
    Hmax = (Hmax + x) % 180;
    Hmin = (Hmin + 180 - x) % 180;

    // Saturation: 0-255
    Smin = Smax = (int) colorHSV.val[1];
    x = (int) rangeHSV.val[1];
    Smax += x;
    if (Smax > 255)
        Smax = 255;
    Smin -= x;
    if (Smin < 0)
        Smin = 0;

    // Value: 0-255
    Vmin = Vmax = (int) colorHSV.val[2];
    x = (int) rangeHSV.val[2];
    Vmax += x;
    if (Vmax > 255)
        Vmax = 255;
    Vmin -= x;
    if (Vmin < 0)
        Vmin = 0;

    // threshold H plane
    if (Hmax >= Hmin) {
        cv::inRange(srcHSV, cv::Scalar(Hmin, Smin, Vmin),
                cv::Scalar(Hmax, Smax, Vmax), dstBin);
    } else {
        static cv::Mat tmp;
        tmp = cvCreateImageOnce(tmp, dstBin.size(), 8, 1, false);
        cv::inRange(srcHSV, cv::Scalar(Hmin, Smin, Vmin),
                cv::Scalar(255, Smax, Vmax), dstBin);
        cv::inRange(srcHSV, cv::Scalar(0, Smin, Vmin),
                cv::Scalar(Hmax, Smax, Vmax), tmp);
        cv::bitwise_or(tmp, dstBin, dstBin);
    }
}

void cvSkeleton(cv::Mat &src, cv::Mat &dst) {
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
    cv::Mat temp(src.size(), CV_8UC1);
    cv::Mat eroded(src.size(), CV_8UC1);
    cv::Mat skel(src.size(), CV_8UC1);

    src.copyTo(dst);

    do {
        cv::erode(dst, eroded, element);
        cv::dilate(eroded, temp, element);
        cv::subtract(dst, temp, temp);
        cv::bitwise_or(skel, temp, skel);
        eroded.copyTo(dst);
    } while (cv::norm(dst));

    skel.copyTo(dst);
}

void FilterMotion(cv::Mat &srcColor, cv::Mat &movingAverage,
        cv::Mat &dstGrey, double mdAlpha, int mdThreshold) {
    std::vector<cv::Mat> channels;
    cv::Mat difference(srcColor.size(), CV_8UC3);

    //Convert high-res moving average and store in difference.
    movingAverage.convertTo(difference, CV_8UC3);

    //Substract the current frame from the moving average.
    cv::absdiff(srcColor, difference, difference);

    // update running average with current frame
    cv::accumulateWeighted(srcColor, movingAverage, mdAlpha);

    // split difference image into channels
    cv::split(difference, channels);

	// add individual channel differences
	cv::add(channels[0], channels[1], dstGrey); // b + g
	cv::add(channels[2], dstGrey, dstGrey); // + r
	// or choose max deviation
	// (this should be used for PROJECT_MAZE)
	// cv::Max(channels[0], channels[1], dstGrey); // b + g
    // cv::Max(channels[2], dstGrey, dstGrey); // + r

    // debug output: average difference on new image
    //cv::Scalar avgdiff = cv::Avg(dstGrey);
    //ofslog << currentframe << "\tAVGDIFF\t" << avgdiff.val[0] << endl;

    //Convert the image to grayscale.
    //cv::cvtColor(difference,dstGrey,CV_RGB2GRAY);

    //Convert the image to black and white.
    cv::threshold(dstGrey, dstGrey, mdThreshold, 255, cv::THRESH_BINARY);

    //Dilate and erode to get moving blobs
    //TODO: these parameters can be optimized, too/
    cv::dilate(dstGrey, dstGrey, cv::Mat(), cv::Point(-1,-1), 6);
    cv::erode(dstGrey, dstGrey, cv::Mat(), cv::Point(-1,-1), 4);
}

void FitLine(cv::Point *points, int count, float *line) {
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
