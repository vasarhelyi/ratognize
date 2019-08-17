#include <iostream>
#include <list>

#include "color.h"
#include "cvutils.h"
#include "log.h"

bool compareCColorSetsByDate(const cColorSet & a, const cColorSet & b) {
    return (a.datetime < b.datetime);
}

void InterpolateColors(tColor* result, tColor* first, tColor* second, double wprev, double w) {
	// Hue (0-180)
    double d = second->mColorHSV.val[0];
    double dd = first->mColorHSV.val[0];
    if (abs(d - dd) > 90) {
        if (d < dd)
            d += 180;
        else
            dd += 180;
    }
    result->mColorHSV.val[0] =
            (int) (w * d + wprev * dd) % 180;
    result->mRangeHSV.val[0] =
            (int) (w * second->mRangeHSV.val[0] +
            wprev * first->mRangeHSV.val[0]);
    // Saturation
    result->mColorHSV.val[1] =
            (int) (w * second->mColorHSV.val[1] +
            wprev * first->mColorHSV.val[1]);
    result->mRangeHSV.val[1] =
            (int) (w * second->mRangeHSV.val[1] +
            wprev * first->mRangeHSV.val[1]);
    // Value
    result->mColorHSV.val[2] =
            (int) (w * second->mColorHSV.val[2] +
            wprev * first->mColorHSV.val[2]);
    result->mRangeHSV.val[2] =
            (int) (w * second->mRangeHSV.val[2] +
            wprev * first->mRangeHSV.val[2]);
}

bool SetHSVDetectionParams(lighttype_t light, color_interpolation_t method,
        int dslp, double inputvideostarttime, std::list<cColorSet>* mColorDataBase, 
		cColor* mColor, tColor* mBGColor) {
	// TODO: set extra code for EXTRALIGHT, STRANGELIGHT.
    // we use daylight for extra, nightlight for strange.
    switch (light) {
    case DAYLIGHT:
    case NIGHTLIGHT:
        // NOP
        break;
    case EXTRALIGHT:
        light = DAYLIGHT;
        break;
    case STRANGELIGHT:
        light = NIGHTLIGHT;
        break;
    case UNINITIALIZEDLIGHT:
        LOG_ERROR("There is no code implemented for UNINITIALIZED light.");
        return false;
    }

    std::list < cColorSet >::iterator it = mColorDataBase[light].begin();

    // regardless of method, check if there is only one entry which matches dayssincelastpaint
    if (mColorDataBase[light].size() == 1 && (*it).day == dslp) {
        for (int i = 0; i < MAXMBASE; i++)
            mColor[i].mColor = (*it).mColor[i];
		*mBGColor =  (*it).mBGColor;
		return true;
    }
    ////////////////////////////////////////////////////////////////////////
    if (method == COLOR_INTERPOLATE) {
        // get color from database with current paint age
        // databases are (and must be) pre-sorted according to increasing day value
        while (it != mColorDataBase[light].end()
                && (*it).day < dslp) {
            std::cout << (*it).day << std::endl;
            it++;
        }
        // error, dayssincelastpaint is too big
        if (it == mColorDataBase[light].end()) {
            LOG_ERROR("Dayssincelastpaint is too big, could not interpolate using mColorDataBase.");
            return false;
        }
        // exact match, simple copy
        else if ((*it).day == dslp) {
            for (int i = 0; i < MAXMBASE; i++) {
                mColor[i].mColor = (*it).mColor[i];
			}
			*mBGColor =  (*it).mBGColor;
		}
        // error, even the first entry is greater then current day
        else if (it == mColorDataBase[light].begin()) {
            LOG_ERROR("Dayssincelastpaint is too small, could not interpolate using mColorDataBase.");
            return false;
        }
        // linear interpolate between (it) and (it-1)
        else {
            std::list < cColorSet >::iterator itprev = it;
            itprev--;           // previous element
            int j = (*it).day - (*(itprev)).day;        // length
            double w = (double) (dslp - (*itprev).day) / j;    // weight
            double wprev = (double) ((*it).day - dslp) / j;    // weight prev
            for (int i = 0; i < MAXMBASE; i++) {
				InterpolateColors(&mColor[i].mColor, &(*itprev).mColor[i], 
						&(*it).mColor[i], wprev, w);
            }
			// Background colors are also interpolated
			InterpolateColors(mBGColor, &(*itprev).mBGColor, 
						&(*it).mBGColor, wprev, w);
        } // interpolate
        ////////////////////////////////////////////////////////////////////////
    } else if (method == COLOR_FIT_LINEAR) {
        // init variables
        if ((int) mColorDataBase[light].size() < 2) {
            LOG_ERROR("Too few points in color dataspace, could not fit line!");
            return false;
        }
        CvPoint *points =
                (CvPoint *) malloc(mColorDataBase[light].size() *
                sizeof(points[0]));
        //CvPoint2D32f* points = (CvPoint2D32f*)malloc(mColorDataBase[light].size() * sizeof(points[0]));
        //CvMat pointMat =
        //        cvMat(1, (int) mColorDataBase[light].size(), CV_32SC2, points);
        float line[4];
        // iterate all colors (and background when i == MAXMBASE)
        for (int i = 0; i <= MAXMBASE; i++) {
            if (i < MAXMBASE && !mColor[i].mUse)
                continue;
            // iterate HSV
            for (int j = 0; j < 3; j++) {
                // fill points (x = day, y = color value)
                int k = 0, avgrange = 0, offset = 0;
                if (j == 0)     // Hue, circular in 180, if needed set +180 offset for small part
                {
                    int minx = 180, maxx = 0;
                    for (it = mColorDataBase[light].begin();
                            it != mColorDataBase[light].end(); ++it) {
                        // blob colors
						if (i < MAXMBASE) {
							if ((int) ((*it).mColor[i].mColorHSV.val[j]) < minx) {
								minx = (int) ((*it).mColor[i].mColorHSV.val[j]);
							}
							if ((int) ((*it).mColor[i].mColorHSV.val[j]) > maxx) {
								maxx = (int) ((*it).mColor[i].mColorHSV.val[j]);
							}
						// background color
						} else {
							if ((int) ((*it).mBGColor.mColorHSV.val[j]) < minx) {
								minx = (int) ((*it).mBGColor.mColorHSV.val[j]);
							}
							if ((int) ((*it).mBGColor.mColorHSV.val[j]) > maxx) {
								maxx = (int) ((*it).mBGColor.mColorHSV.val[j]);
							}
						}
                    }
                    // possibly overflowing colors, average them in 90-270 range
                    if (maxx - minx > 90)
                        offset = 180;
                }
                for (it = mColorDataBase[light].begin();
                        it != mColorDataBase[light].end(); ++it) {
                    // x
                    points[k].x = (*it).day;
					if (i < MAXMBASE) {
						// y
						points[k].y = (int) ((*it).mColor[i].mColorHSV.val[j]);     // Saturation, Value, no overflowing Hue
						if (offset && points[k].y < 90)
							points[k].y += offset;  // overflowing Hue in 90-270 range
						// range
						avgrange += (int) ((*it).mColor[i].mRangeHSV.val[j]);
					} else {
						// y
						points[k].y = (int) ((*it).mBGColor.mColorHSV.val[j]);     // Saturation, Value, no overflowing Hue
						if (offset && points[k].y < 90)
							points[k].y += offset;  // overflowing Hue in 90-270 range
						// range
						avgrange += (int) ((*it).mBGColor.mRangeHSV.val[j]);
					}
                    // count
                    k++;
                }
                /*
                   // fit line on points. TODO Problem: distance is proportional to r, not dy!!!
                   cvFitLineFixX( &pointMat, CV_DIST_L1, 0, 0.01, 0.01, line );
                   //cvFitLine2D( points, k, CV_DIST_L1, 0, 0.0001, 0.0001, line );
                   // store color approximation from linear fit and average range
                   if (line[0] == 0)
                   {
                   LOG_ERROR("line fit error, vx is zero, line is vertical.");
                   free( points );
                   return false;
                   }
                   if (i < MAXMBASE) {
				       mColor[i].mColor.mColorHSV.val[j] = (int)(line[3] + line[1]/line[0] * (dslp-line[2]));
				   } else {
				       mBGColor->mColorHSV.val[j] = (int)(line[3] + line[1]/line[0] * (dslp-line[2]));
				   }
				   
                 */
                // fit line on points
                FitLine(points, k, line);
				// blob colors
				if (i < MAXMBASE) {
					mColor[i].mColor.mColorHSV.val[j] =
					        (int) (line[0] + line[1] * dslp);
					if (offset) {
						mColor[i].mColor.mColorHSV.val[j] =
								((int) mColor[i].mColor.mColorHSV.val[j] % offset);
					}
					mColor[i].mColor.mRangeHSV.val[j] = avgrange / k;
				// background color
				} else {
					mBGColor->mColorHSV.val[j] =
					        (int) (line[0] + line[1] * dslp);
					if (offset) {
						mBGColor->mColorHSV.val[j] =
								((int) mBGColor->mColorHSV.val[j] % offset);
					}
					mBGColor->mRangeHSV.val[j] = avgrange / k;
				}
            }                   // j, HSV
        }                       // colors
        free(points);
        ////////////////////////////////////////////////////////////////////////
    } else if (method == COLOR_INTERPOLATE_DATE) {
        // databases are (and must be) pre-sorted according to increasing datetime (time_t) value
        while (it != mColorDataBase[light].end()
                && (*it).datetime < inputvideostarttime) {
            std::cout << (*it).datetime << std::endl;
            it++;
        }
        // error, inputvideostarttime is after last color entry
        if (it == mColorDataBase[light].end()) {
            LOG_ERROR("Inputvideostarttime is too big, could not interpolate using mColorDataBase.");
            return false;
            // error, even the first entry is greater then inputvideostarttime
        } else if (it == mColorDataBase[light].begin()
                && (*it).datetime > inputvideostarttime) {
            LOG_ERROR("Inputvideostarttime is too small, could not interpolate using mColorDataBase.");
            return false;
        }
        // exact day match, simple copy
        struct tm *temptm1, *temptm2;
        time_t temptimet2 = (time_t) inputvideostarttime;
        temptm1 = localtime(&(*it).datetime);
        temptm1->tm_sec = temptm1->tm_min = temptm1->tm_hour = 0;
        temptm2 = localtime(&temptimet2);
        temptm2->tm_sec = temptm2->tm_min = temptm2->tm_hour = 0;
        if (mktime(temptm1) == mktime(temptm2)) {
            for (int i = 0; i < MAXMBASE; i++)
                mColor[i].mColor = (*it).mColor[i];
            return true;
        }
        // linear interpolate between (it) and (it-1)
        std::list < cColorSet >::iterator itprev = it;
        itprev--;               // previous element
        int j = (int) ((*it).datetime - (*(itprev)).datetime);  // length in sec
        double w = (double) (inputvideostarttime - (*itprev).datetime) / j;     // weight
        double wprev = (double) ((*it).datetime - inputvideostarttime) / j;     // weight prev
        for (int i = 0; i < MAXMBASE; i++) {
			InterpolateColors(&mColor[i].mColor, &(*itprev).mColor[i], 
					&(*it).mColor[i], wprev, w);
        }
		// Background colors are also interpolated
		InterpolateColors(mBGColor, &(*itprev).mBGColor, 
					&(*it).mBGColor, wprev, w);
    } // switch method

    // return without errror
    return true;
}
