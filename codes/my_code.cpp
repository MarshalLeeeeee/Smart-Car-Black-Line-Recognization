#include "stdafx.h"
#include <opencv\\highgui.h>
#include <opencv\\cv.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include "car.h"
#include "ImageClass.h"

using namespace cv;
using namespace std;

enum { height = 400, width = 400 };  // the size of the window
car mycar;  // the car 
CvPoint2D32f originpoints[5]; // the origin edge of the origin pic
CvPoint2D32f newpoints[4];  // the new edge of the new pic
CvPoint2D32f corners[50];
int h = 0;  // to count the time
CvScalar upper, lower;   //CVSCALAR 可以存放四个double变量的数组 r,g,b,a四个通道
CvCapture *cam;
IplImage *transimg = 0, *img;
IplImage *TrackImage = cvCreateImage(cvSize(400, 400), IPL_DEPTH_8U, 3);
IplImage* dstimg = cvCreateImage(cvSize(400, 400), IPL_DEPTH_8U, 1);
IplImage* dstimg2 = cvCreateImage(cvSize(400, 400), IPL_DEPTH_8U, 1);
CvMat* transmat;


void adjust(void)
{
	int xmin = 10000, ymin = 10000;
	int xmax = 0, ymax = 0;
	for (int index = 0; index < 4; index++)
	{
		if (xmin > originpoints[index].x) xmin = originpoints[index].x;
		if (xmax < originpoints[index].x) xmax = originpoints[index].x;
	}
	for (int index = 0; index < 4; index++)
	{
		if (ymin > originpoints[index].y) ymin = originpoints[index].y;
		if (ymax < originpoints[index].y) ymax = originpoints[index].y;
	}
	originpoints[0] = cvPoint2D32f(xmin, ymin);
	originpoints[1] = cvPoint2D32f(xmax, ymin);
	originpoints[2] = cvPoint2D32f(xmin, ymax);
	originpoints[3] = cvPoint2D32f(xmax, ymax);
}
void cvThin(IplImage* src, IplImage* dst, int iterations = 1)
{
	cvCopyImage(src, dst);
	BwImage dstdat(dst);
	IplImage* t_image = cvCloneImage(src);
	BwImage t_dat(t_image);
	for (int n = 0; n < iterations; n++)
	for (int s = 0; s <= 1; s++)
	{
		cvCopyImage(dst, t_image);
		for (int i = 0; i < src->height; i++)
		for (int j = 0; j < src->width; j++)
		if (t_dat[i][j])
		{
			int a = 0, b = 0;
			int d[8][2] = { { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 } };
			int p[8];
			p[0] = (i == 0) ? 0 : t_dat[i - 1][j];
			for (int k = 1; k <= 8; k++)
			{
				if (i + d[k % 8][0] < 0 || i + d[k % 8][0] >= src->height || j + d[k % 8][1] < 0 || j + d[k % 8][1] >= src->width)
					p[k % 8] = 0;
				else p[k % 8] = t_dat[i + d[k % 8][0]][j + d[k % 8][1]];
				if (p[k % 8])
				{
					b++;
					if (!p[k - 1]) a++;
				}
			}
			if (b >= 2 && b <= 6 && a == 1)
			if (!s && !(p[2] && p[4] && (p[0] || p[6])))
				dstdat[i][j] = 0;
			else if (s && !(p[0] && p[6] && (p[2] || p[4])))
				dstdat[i][j] = 0;
		}
	}
	cvReleaseImage(&t_image);
}

void mouse_click(int mouseevent, int x, int y, int flags, void* param)
{
	// to match cvMouseCallBack
	if (mouseevent == CV_EVENT_LBUTTONDOWN)    // click left mouse
	{
		originpoints[h] = cvPoint2D32f(x, y);
		if (h<4) ++h;
		std::cout << x << " " << y << std::endl;
	}
}


CvPoint2D32f nextPoint(CvPoint2D32f* points, CvPoint2D32f center)
{
	// find the nearest point
	double min = 100000, tmp;
	int minN = 29;
	CvPoint2D32f nextP;
	nextP.x = 0; nextP.y = 0;
	CvPoint tmpPoint;
	for (int i = 0; i<50; i++)
	{
		tmpPoint = cvPointFrom32f(points[i]);
		if (tmpPoint.x != 0 || tmpPoint.y != 0)
		{
			tmp = sqrt((points[i].x - center.x)*(points[i].x - center.x) + (points[i].y - center.y)*(points[i].y - center.y));
			if (tmp<min) { min = tmp; nextP.x = points[i].x; nextP.y = points[i].y; minN = i; }
		}
	}
	points[minN].x = 0; points[minN].y = 0;

	return nextP;
}

#define PI 3.1415926535

//求两整数点距离
double getdistance(CvPoint2D32f firpoint, CvPoint2D32f secpoint)
{
	double distance;
	distance = sqrtl((firpoint.x - secpoint.x)*(firpoint.x - secpoint.x) + (firpoint.y - secpoint.y)*(firpoint.y - secpoint.y));
	return distance;
}

double getangle(CvPoint2D32f firpoint, CvPoint2D32f secpoint)
{
	// return the angle
	double angle;
	double distance;

	distance = getdistance(firpoint, secpoint);
	angle = acosl((secpoint.x - firpoint.x) / distance) * 180 / PI;
	if (secpoint.y <= firpoint.y)
		angle = 360 - acosl((secpoint.x - firpoint.x) / distance) * 180 / PI;
	else
		angle = acosl((secpoint.x - firpoint.x) / distance) * 180 / PI;
	return angle;
}

int main()
{
	//get image from the camera
	//if failed assert the program
	cam = cvCreateCameraCapture(0);
	if (!cam) return -1;

	cvNamedWindow("win1");
	cvNamedWindow("win2");
	cvSetMouseCallback("win1", mouse_click);

	h = 0;
	while (h<4) // wait for 4 click events
	{
		img = cvQueryFrame(cam); // point to the image which captured by the camera
		cvShowImage("win1", img);
		cvWaitKey(1);// wait for mouse click, and will go on even if no feedback
	}

	transimg = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);// 8 bit pic
	transmat = cvCreateMat(3, 3, CV_32FC1);
	newpoints[0] = cvPoint2D32f(0, 0);
	newpoints[1] = cvPoint2D32f(400, 0);
	newpoints[2] = cvPoint2D32f(0, 400);
	newpoints[3] = cvPoint2D32f(400, 400);
	//adjust();

	while (1)      // matrix transformation
	{
		img = cvQueryFrame(cam);
		cvGetPerspectiveTransform(originpoints, newpoints, transmat);   // calc the transform matrix and save it into transmat
		cvWarpPerspective(img, transimg, transmat);  // get new image according to the transport matrix
		cvShowImage("win1", transimg);
		if (cvWaitKey(1) == 13) break; // if feedback is [ENTER], turn to naxt step
	}

	// deal the image
	img = cvQueryFrame(cam);
	cvGetPerspectiveTransform(originpoints, newpoints, transmat);
	cvWarpPerspective(img, transimg, transmat);
	cvInRangeS(transimg, CV_RGB(0, 0, 0), CV_RGB(50, 50, 50), dstimg);  // alter to 1 bit, save into dstimg
	cvSmooth(dstimg, dstimg2, CV_GAUSSIAN); // gaussian blur
	cvThin(dstimg2, dstimg, 20);// thinen one bit pic

	// create route
	int cornerCounts = 30;
	for (int i = 0; i<50; i++) // initialize the array
	{
		corners[i].x = 0;
		corners[i].y = 0;
	}
	IplImage* tmp1 = cvCreateImage(cvSize(400, 400), IPL_DEPTH_32F, 1);
	IplImage* tmp2 = cvCreateImage(cvSize(400, 400), IPL_DEPTH_32F, 1);
	cvGoodFeaturesToTrack(dstimg, tmp1, tmp2, corners, &cornerCounts, 0.05, 40);  //ShiTomasi corner point
	cvMerge(dstimg, dstimg, dstimg, 0, TrackImage);
	for (int i = 0; i <cornerCounts; ++i)
	{
		if (corners[i].x<390 && corners[i].x>10 && corners[i].y>10 && corners[i].y<390)
			cvCircle(TrackImage, cvPoint((int)(corners[i].x), (int)(corners[i].y)), 6, CV_RGB(255, 0, 0), 2);
	}
	cvReleaseImage(&tmp1); cvReleaseImage(&tmp2);


	// test 
	for (int i = 0; i<50; i++)
	{
		if (corners[i].x<10 || corners[i].x>390 || corners[i].y<10 || corners[i].y>390)
		{
			corners[i].x = 0; corners[i].y = 0;
		}
		std::cout << int(corners[i].x) << ' ' << int(corners[i].y) << std::endl;
	}


	cvShowImage("win1", TrackImage);

	int Nred = 0, Nblue = 0; //red is the tail, blue is the head
	int sumx = 0, sumy = 0;
	CvPoint2D32f Cred, Cblue, mid;

	img = cvQueryFrame(cam);
	cvGetPerspectiveTransform(originpoints, newpoints, transmat);
	cvWarpPerspective(img, transimg, transmat);

	IplImage *dst_image = cvCreateImage(cvGetSize(transimg), 32, transimg->nChannels);
	IplImage *src_image_32 = cvCreateImage(cvGetSize(transimg), 32, transimg->nChannels);
	HsvFloatImage HSVimg(dst_image);

	while (1)
	{
		img = cvQueryFrame(cam);
		cvGetPerspectiveTransform(originpoints, newpoints, transmat);
		cvWarpPerspective(img, transimg, transmat);

		cvConvertScale(transimg, src_image_32);
		cvCvtColor(src_image_32, dst_image, CV_BGR2HSV);

		Nred = 0; Nblue = 0;

		// find the red
		sumx = 0; sumy = 0;
		for (int i = 0; i<400; ++i){               
			for (int j = 0; j<400; ++j){
				if (((HSVimg[i][j].h>-1 && HSVimg[i][j].h<10) || (HSVimg[i][j].h>350 && HSVimg[i][j].h<361)) && HSVimg[i][j].s>0.4   && HSVimg[i][j].v>60)
				{
					sumx += j; sumy += i; ++Nred;
				}
			}
		}
		if (Nred == 0) 	Nred = 1;
		Cred.x = sumx / Nred; Cred.y = sumy / Nred;

		// find the blue
		sumx = 0; sumy = 0;
		for (int i = 0; i<400; ++i){
			for (int j = 0; j<400; ++j){
				if ((HSVimg[i][j].h>220 && HSVimg[i][j].h<230) && HSVimg[i][j].s>0.4   && HSVimg[i][j].v>60)
				{
					sumx += j; sumy += i; ++Nblue;
				}
			}
		}
		if (Nblue == 0) 	Nblue = 1;
		Cblue.x = sumx / Nblue; Cblue.y = sumy / Nblue;

		// point them out
		cvCircle(transimg, cvPoint((int)(Cred.x), (int)(Cred.y)), 15, CV_RGB(255, 0, 0), 4);
		cvCircle(transimg, cvPoint((int)(Cblue.x), (int)(Cblue.y)), 15, CV_RGB(0, 0, 255), 4);

		cvShowImage("win2", transimg);

		if (cvWaitKey(1) == 13) break;// wait [ENTER] to start work
	}

	// yaw: the corner of the car tail and head according to the x axis;
	// direction: the corner of the car and the next point;
	// tmpDistance: the distance to the next point
	double yaw, direction, minusangle, tmpDistance = 0;
	CvPoint2D32f next;

	std::cout << "Point: " << std::endl;
	for (int i = 0; i<50; i++)
		std::cout << corners[i].x << ' ' << corners[i].y << endl;

	while (1)
	{
		img = cvQueryFrame(cam);
		cvGetPerspectiveTransform(originpoints, newpoints, transmat);
		cvWarpPerspective(img, transimg, transmat);

		cvConvertScale(transimg, src_image_32);
		cvCvtColor(src_image_32, dst_image, CV_BGR2HSV);

		Nred = 0; Nblue = 0;

		// find the red
		sumx = 0; sumy = 0;
		for (int i = 0; i<400; ++i){
			for (int j = 0; j<400; ++j){
				if (((HSVimg[i][j].h>-1 && HSVimg[i][j].h<10) || (HSVimg[i][j].h>350 && HSVimg[i][j].h<361)) && HSVimg[i][j].s>0.4   && HSVimg[i][j].v>60)
				{
					sumx += j; sumy += i; ++Nred;
				}
			}
		}
		if (Nred == 0) 	Nred = 1;
		Cred.x = sumx / Nred; Cred.y = sumy / Nred;

		// find the blue
		sumx = 0; sumy = 0;
		for (int i = 0; i<400; ++i){
			for (int j = 0; j<400; ++j){
				if ((HSVimg[i][j].h>220 && HSVimg[i][j].h<230) && HSVimg[i][j].s>0.4   && HSVimg[i][j].v>60)
				{
					sumx += j; sumy += i; ++Nblue;
				}
			}
		}
		if (Nblue == 0) 	Nblue = 1;
		Cblue.x = sumx / Nblue; Cblue.y = sumy / Nblue;
		cvCircle(transimg, cvPoint((int)(Cred.x), (int)(Cred.y)), 15, CV_RGB(255, 0, 0), 4);
		cvCircle(transimg, cvPoint((int)(Cblue.x), (int)(Cblue.y)), 15, CV_RGB(0, 0, 255), 4);


		// if the distance is less than some figure, go to the next
		if (tmpDistance<50) next = nextPoint(corners, Cblue);

		if (int(next.x) == 0 && int(next.y == 0))break; // Over!
		mid.x = (Cred.x + Cblue.x) / 2;
		mid.y = (Cred.y + Cblue.y) / 2;

		tmpDistance = sqrt((next.x - mid.x)*(next.x - mid.x) + (next.y - mid.y)*(next.y - mid.y));
		yaw = getangle(Cred, Cblue);
		direction = getangle(mid, next);
		minusangle = yaw - direction;
		if (minusangle < -180) minusangle += 360;
		else if (minusangle >180) minusangle -= 360;
		cout << yaw << ' ' << direction << ' ' << minusangle << endl;
		if (minusangle<-9 && minusangle>-90) { cout << minusangle << " right  "; mycar.turnr(); }// turn right
		else if (minusangle>9 && minusangle<90) { cout << minusangle << " left  "; mycar.turnl(); }// turn left
		else if (minusangle<-90 || minusangle>90) { cout << minusangle << " back  "; mycar.back(); mycar.back(); mycar.back(); }// go backward
		else { cout << minusangle << " run  "; mycar.run(); }// go forward

		cvCircle(TrackImage, cvPoint((int)(next.x), (int)(next.y)), 6, CV_RGB(0, 0, 255), 2);
		cvCircle(transimg, cvPoint((int)(next.x), (int)(next.y)), 6, CV_RGB(255, 255, 255), 2);

		cvShowImage("win1", TrackImage);		 
		cvShowImage("win2", transimg);
		if (cvWaitKey(1) == 13) break;
	}
	mycar.stop();
	cout << "Over！" << endl;
	system("pause");

	return 0;
}