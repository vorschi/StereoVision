// test.cpp : Definiert den Einstiegspunkt f�r die Konsolenanwendung.

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include "guidedfilter.h"
#include <opencv2\opencv.hpp>
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

void convertToGrayscale(const Mat &img, Mat &imgGray){
	
	for (int x=0; x<img.cols; x++){		//for-loops for going through picture per pixel
		for (int y=0; y<img.rows; y++){
			
			Vec3b intensity = img.at<Vec3b>(y, x);		//calculating luminance value for each pixel
			uchar blue = intensity.val[0];				//and pass it to the grayscale image
			uchar green = intensity.val[1];
			uchar red = intensity.val[2];	
			uchar grey = 0.21*red+0.72*green+0.07*blue;
			
			imgGray.at<uchar>(y, x) = grey;			
		}
	}	
}

void computeCostVolume(const Mat &imgLeft, const Mat &imgRight, vector<Mat> &costVolumeLeft, 
	vector<Mat> &costVolumeRight, int windowSize, int maxDisp){

		
	int borderSize = windowSize/2;

	Mat tempLeft(imgLeft.rows, imgLeft.cols, CV_32FC1);		//matrices for storing cost volume temporarily
	Mat tempRight(imgRight.rows, imgRight.cols, CV_32FC1);

	float costLeft = 0;
	float costRight = 0;

	float normLeft = 0;
	float normRight = 0;



	int windowArea = windowSize*windowSize;

	for(int i=0; i<=maxDisp; i++){		// for-loop for disparity values

		for (int x=0; x<imgLeft.cols; x++){		//for-loops for going through picture per pixel
			for (int y=0; y<imgLeft.rows; y++){

				for (int a=windowSize/2; a>=(-windowSize/2); a--){		//for-loops for scanning window
					for (int  b=windowSize/2; b>=(-windowSize/2); b--){

						//if((x>i)&&(y>windowSize/2)&&((y+windowSize/2)<imgLeft.rows)&&((x+windowSize/2)<imgLeft.cols)){			//handling boundary problems left
							if(((x-a)>0)&&((x-a)<imgLeft.cols)&&((y-b)>0)&&((y-b)<imgLeft.rows)){
				
								if((x-a-i)<0){		//setting costVolume very high for out-of-image-windows
									costLeft+=1000; 
								}
								else{				//calculating costVolums
									costLeft+=abs(imgLeft.at<uchar>(y-b,x-a) - imgRight.at<uchar>(y-b,x-a-i));
									normLeft++;
								}
							}
						

						//if((x>i)&&(y>windowSize/2)&&((y+windowSize/2)<imgRight.rows)&&((x+windowSize/2)<imgRight.cols)){			//handling boundary problems right
							if(((x-a)>0)&&((x-a)<imgLeft.cols)&&((y-b)>0)&&((y-b)<imgLeft.rows)){
								
								if((x-a+i)>=imgLeft.cols){		//setting costVolume very high for out-of-image-windows
									costRight+=1000;
								}
								else{				//calculating costVolums
									costRight+=abs(imgRight.at<uchar>(y-b,x-a) - imgLeft.at<uchar>(y-b,x-a+i));
									normRight++;
								}

							}
						
					}		
				}

				if (normLeft==0)			
					costLeft = costLeft / normLeft;		//normalizing cost volume
				if (normRight==0)
					costRight = costRight / normRight;

				if (costRight>255)
					costRight=255;
				if (costLeft>255)
					costLeft=255;

				tempLeft.at<float>(y,x) = costLeft;			//write cost volume in temporary matrix and reset it
				tempRight.at<float>(y,x) = costRight;
				costLeft = 0;
				costRight = 0;
				normLeft=0;
				normRight=0;
			}
		}
		costVolumeLeft.push_back(tempLeft);			//writing cost Volume matrix in vector and reset it
		costVolumeRight.push_back(tempRight);

		Mat temp2(imgLeft.rows, imgLeft.cols, CV_32FC1);
		Mat temp3(imgRight.rows, imgRight.cols, CV_32FC1);
		tempLeft = temp2;
		tempRight = temp3;
	}
}

void selectDisparity(Mat &dispLeft, Mat &dispRight, vector<Mat> &costVolumeLeft, 
	vector<Mat> &costVolumeRight, int scaleDispFactor){
	
	uchar displevelLeft = 255;
	uchar displevelRight = 255;

	int disperityLeft = 0;
	int disperityRight = 0;

	float costVolLeft = 0;
	float costVolRight = 0;

	for (int x=0; x<dispLeft.cols; ++x){		//for-loops for going through picture per pixel
		for (int y=0; y<dispLeft.rows; ++y){

			for(int i=0; i<costVolumeLeft.size(); i++){			//for-loops for different disparity values

				costVolLeft = costVolumeLeft.at(i).at<float>(y,x);
				costVolRight = costVolumeRight.at(i).at<float>(y,x);

				if (costVolLeft<displevelLeft){				//check if current cost volume is lower then previously lowest
				  displevelLeft = costVolumeLeft.at(i).at<float>(y,x);			//if so, set it as new lower boundary
				  disperityLeft = i;
				}  

				if (costVolRight<displevelRight){
					displevelRight = costVolumeRight.at(i).at<float>(y,x);
					disperityRight = i;
				}
			}

			dispLeft.at<uchar>(y,x)=disperityLeft*scaleDispFactor;			//set pixel in desparity map
			dispRight.at<uchar>(y,x)=disperityRight*scaleDispFactor;			//set pixel in desparity map
			disperityLeft = 0;
			disperityRight = 0;
			displevelLeft = 255;
			displevelRight = 255;
		}
	}
}

void aggregateCostVolume(const cv::Mat &imgLeft, const cv::Mat &imgRight, std::vector<cv::Mat>
&costVolumeLeft, std::vector<cv::Mat> &costVolumeRight, int r, double eps){

eps *= 255 * 255;   
Mat tempLeft(imgLeft.rows, imgLeft.cols, CV_32FC1);		//matrices for storing cost volume temporarily
Mat tempRight(imgRight.rows, imgRight.cols, CV_32FC1);
imgLeft.convertTo(tempLeft, CV_32F);
imgRight.convertTo(tempRight, CV_32F);

for(int i=0; i<costVolumeLeft.size(); i++){
	costVolumeLeft.at(i) = guidedFilter(imgLeft, costVolumeLeft[i], r, eps);
	costVolumeRight.at(i) = guidedFilter(costVolumeRight[i], imgRight, r, eps);
}
}

void refineDisparity(cv::Mat &dispLeft, cv::Mat &dispRight, int scaleDispFactor){

}

int main(){
	const Mat imgLeft = imread("tsukuba_left.png", CV_LOAD_IMAGE_UNCHANGED);		//reading the two original images
	const Mat imgRight = imread("tsukuba_right.png", CV_LOAD_IMAGE_UNCHANGED);
	
	vector<Mat> costVolumeLeft;			//vectors for storing the different cost volumes for each disparity
	vector<Mat> costVolumeRight;
	
	int windowSize = 3;			//scanning window size
	int maxDisp = 15;			//definition of the maximum disparity

	Mat dispLeft(imgLeft.rows, imgLeft.cols, CV_8UC1);
	Mat dispRight(imgRight.rows, imgRight.cols, CV_8UC1);

	int r = 2;
	double eps = 0.1 * 0.1;

	int scaleDispFactor = 16;		//stretches the disparity levels for better visualization

	Mat imgGrayLeft(imgLeft.rows, imgLeft.cols, CV_8UC1);			//computing the grayscale images				
	Mat imgGrayRight(imgRight.rows, imgRight.cols, CV_8UC1);

	convertToGrayscale(imgLeft, imgGrayLeft);
	convertToGrayscale(imgRight, imgGrayRight);

	computeCostVolume(imgGrayLeft, imgGrayRight, costVolumeLeft, costVolumeRight, windowSize, maxDisp);

	aggregateCostVolume(imgLeft, imgRight, costVolumeLeft, costVolumeRight, r, eps);

	selectDisparity(dispLeft, dispRight, costVolumeLeft, costVolumeRight, scaleDispFactor);

	imshow("dispLeft",dispLeft);
	//imshow("dispRight",dispRight);

	waitKey(0);
	return 0;
}