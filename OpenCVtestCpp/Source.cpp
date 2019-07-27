#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <time.h>

#include <string>
#include <fstream>
using namespace std;
using namespace cv;

void Masking(Mat & src) {
	Mat mask;
	cvtColor(src, mask, COLOR_BGR2HSV);
	inRange(mask, Scalar(0, 30, 100), Scalar(255, 250, 255), mask);
	erode(mask, mask, Mat(),Point(-1,-1),1);
	dilate(mask, mask, Mat(),Point(-1,-1),1);
	Mat buffer = Mat(src.rows, src.cols, CV_8U, Scalar(0));
	src.copyTo(buffer, mask);
	src = buffer;
	return;
}

void findCircle(Mat & src) {
	
}

float GetNearestBoalDirection(Mat & src) {
	Masking(src);
	cvtColor(src, src, COLOR_BGR2GRAY);
	vector<Vec3f> circles;
	HoughCircles(src, circles, CV_HOUGH_GRADIENT, 1, 30, 240, 100,30,250);
	for (Vec3f c : circles)circle(src, Point(c[0], c[1]), 15, Scalar(0,0,255),-1);
	cvtColor(src, src, COLOR_BGR2GRAY);
	//medianBlur(src, src, 3);
	GaussianBlur(src, src, Size(3, 3), 1);
	int thresh = 40;
	Canny(src, src, thresh / 2, thresh);
	return 0;
}

class FindCircleTest
{
public:
	int blurTime = 0;
	int dp = 1;
	int minDist = 20;
	int thresh = 100;
	int vote = 100;
	float resizeScale = 1.0f;
	int method = 0;
	bool masking = false;
	bool doFromGRAY = true;
	bool doFromSHINE = false;
	bool doFromHUE = false;
	bool doFromVIVID = false;
	bool useRadius = false;
	bool doResize = false;
	Mat *frame;
	int evaluation = 0;
	vector<Vec3f> circles;
	void FindCircle() {
		switch (method)
		{
		case 0: break;
		case 1:
			doFromGRAY = true;
			doFromSHINE = false;
			doFromHUE = false;
			doFromVIVID = false;
			break;
		case 2:
			doFromGRAY = false;
			doFromSHINE = true;
			doFromHUE = false;
			doFromVIVID = false;
			break;
		case 3:
			doFromGRAY = false;
			doFromSHINE = false;
			doFromHUE = true;
			doFromVIVID = false;
			break;
		case 4:
			doFromGRAY = false;
			doFromSHINE = false;
			doFromHUE = false;
			doFromVIVID = true;
			break;
		}
		circles.clear();
		Mat buffer, src = *frame;
		if (masking) {
			Mat mask;
			buffer = Mat(src.rows, src.cols, CV_8U, Scalar(0));
			cvtColor(src, mask, COLOR_BGR2HSV);
			inRange(mask, Scalar(0, 30, 100), Scalar(255, 250, 255), mask);
			erode(mask, mask, Mat(), Point(-1, -1), 1);
			dilate(mask, mask, Mat(), Point(-1, -1), 1);
			src.copyTo(buffer, mask);
			src = buffer;
		}
		if (doFromGRAY)cvtColor(src,buffer,COLOR_BGR2GRAY);
		else {
			cvtColor(src, buffer, COLOR_BGR2HSV);
			vector<Mat> hsv;
			split(buffer, hsv);
			if (doFromHUE)buffer = hsv[0];
			else if (doFromVIVID)buffer = hsv[1];
			else if (doFromSHINE)buffer = hsv[2];
		}
		if(doResize)resize(buffer, buffer, Size(0, 0), resizeScale, resizeScale);
		for (int i = 0; i < blurTime; i++) blur(buffer, buffer, Size(3, 3));
		HoughCircles(buffer, circles, CV_HOUGH_GRADIENT, dp, minDist, thresh, vote);
		if (doResize) {
			for (int i = 0; i < circles.size(); i++)
			{
				circles[i][0] /= resizeScale;
				circles[i][1] /= resizeScale;
				circles[i][2] /= resizeScale;
			}
		}
		return;
	}
	void FindAndDrawCircle() {
		FindCircle();
		for (Vec3f c : circles)circle(*frame, Point(c[0], c[1]), c[2],Scalar(0,0,255),3);
	}
	void GetEvaluation(vector<Vec3i> & standard) {
		evaluation = 0;
		FindCircle();
		int wrongAns = 0, rightAns = 0;
		bool flag = false;
		for (int c = circles.size() - 1;c > -1; c--) {
			flag = false;
			for (int s = standard.size() - 1; s > -1; s--) {
				if (useRadius) {
					if (abs(circles[c][0] - standard[s][0]) < 15 &&
						abs(circles[c][1] - standard[s][1]) < 15 &&
						abs(circles[c][2] - standard[s][2]) < 15) {
						rightAns++;
						flag = true;
						break;
					}
				}
				else {
					if (abs(circles[c][0] - standard[s][0]) < 15 ||
						abs(circles[c][1] - standard[s][1]) < 15) {
						rightAns++;
						flag = true;
						break;
					}
				}
			}
			if (!flag)wrongAns++;
		}

		evaluation += wrongAns * 10;
		evaluation += abs(standard.size() - rightAns) * 20;
		if (standard.size() > 0 && circles.size() < 1)evaluation += 40;
		return;
	}
};

int counter = 0;
int all;

void LearnArgument(vector<vector<Vec3i>> standard,vector<Mat> frames) {
	vector<FindCircleTest> ress;

	for(int blurTime = 0;blurTime <= 4;blurTime++)
	for(int dp = 1;dp <= 3;dp++)
	for(int minDist = 10;minDist <= 100;minDist += 30)
	for(int thresh = 20;thresh <= 300;thresh += 20)
	for(int method = 1;method <= 3;method++)
	for(int vote = 100;vote >= 20;vote -= 10)
	for(float resizeScale = 0.01f; resizeScale <= 0.5f; resizeScale += 0.2f) {
		FindCircleTest tester;
		tester.blurTime = blurTime;
		tester.dp = dp;
		tester.minDist = minDist;
		tester.thresh = thresh;
		tester.method = method;
		tester.vote = vote;
		tester.doResize = true;
		tester.resizeScale = resizeScale;
		int res = 0;
		for (int i = 0; i < frames.size();i++) {
			tester.frame = &frames[i];
			tester.GetEvaluation(standard[i]);
			res += tester.evaluation;
		}
		counter++;
		if(counter % 100 == 0)cout << 100.0 * (float)counter / (float)all << "%" << endl;
		ress.push_back(tester);
	}
	sort(ress.begin(), ress.end(), [](FindCircleTest &a, FindCircleTest &b) {return a.evaluation < b.evaluation; });

	ofstream file;
	file.open("testdata.txt", ios::out);
	file << " No.  blurTime / dp / method / minDist / resizeScale / thresh / vote / evaluation" << endl;
	for (size_t i = 0; i < 10; i++)
	{
		FindCircleTest tester = ress[i];
		file <<"No." << i << ':' << tester.blurTime << '/' << tester.dp << '/' << tester.method << '/'
		<< tester.minDist << '/' << tester.resizeScale << '/' << tester.thresh << '/'
		<< tester.vote << '/' << tester.evaluation << endl;
	}
	return;
}

VideoCapture cap;

int main() {
	Mat frame;
	cap.open(1);
	if (!cap.isOpened()) {
		cap.open(0);
		if (!cap.isOpened())return -1;
	}

	cap.set(CV_CAP_PROP_FRAME_WIDTH, 400);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 300);
	cap.set(CV_CAP_PROP_HUE, 13);
	
	vector<Mat> teachers;

	for (int i = 1; i <= 9; i++)teachers.push_back(imread(to_string(i) + ".jpg"));
	vector<vector<Vec3i>> balls
		= {{(160,138),(488,185),(968,287)},{(496,347),(155,127),(837,215)}
			,{(171,158),(506,231),(824,222)},{(276,231),(979,299)}
			,{(163,294),(1085,412)},{(701,257),(1133,274)}
			,{(161,178),(820,341),(1119,252)},{(710,295)},{(424,397),(730,388)}};
	//LearnArgument(balls, teachers);

	FindCircleTest tester;
	tester.blurTime = 3;
	tester.dp = 1;
	tester.method = 1;
	tester.minDist = 10;
	tester.resizeScale = 0.4;
	tester.thresh = 70;
	tester.vote = 70;

	for (Mat frame : teachers) {
		tester.frame = &frame;
		tester.FindAndDrawCircle();
		imshow("", *tester.frame);
		waitKey(0);
	}

	/*while (1)
	{
		cap >> frame;
		auto begin = clock();
		tester.frame = &frame;
		tester.FindAndDrawCircle();
		frame = *tester.frame;
		auto end = clock();
		cout << (double)(end - begin) << " ms" << endl;
		imshow("frame", frame);
		if (waitKey(1) > 0)break;
	}*/
	cap.release();
	return 0;
}
//1 1 1 20 0.25 70 50 50