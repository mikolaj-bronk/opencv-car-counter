#include "pch.h"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <ctime>
#include <iomanip>
#include <ctime>
#define CVUI_IMPLEMENTATION
#include <cvui.h>

#define WINDOW_NAME "Menu"
#define MINIMUM_CONTOUR 100000
#define DETECT_BOX_SHIFT_Y 350
#define DETECT_BOX_SHIFT_X 50

using namespace cv;
using namespace std;

vector<Point> addShiftToFrame(vector<Point> vector) 
{
	for (int i = 0; i < vector.size(); i++) {
		vector[i].y = vector[i].y + DETECT_BOX_SHIFT_Y;
		vector[i].x = vector[i].x + DETECT_BOX_SHIFT_X;
	}

	return vector;
}

bool detectAndShowCntours(vector<vector<Point>> contourss, Mat frame, bool show_rectangle)
{
	bool is_detected = false;

	if (contourss.size() > 0) {
		vector<Point> max_contour;
		int largest_area = -1;
		Rect bounding_rect;
		vector<Point2f> mc(contourss.size());

		for (int i = 0; i < contourss.size(); i++) {
			double a = contourArea(contourss[i]);

			if (a > largest_area) {
				largest_area = a;
				max_contour = contourss[i];
			}
		}

		if (largest_area > MINIMUM_CONTOUR) {
			max_contour = addShiftToFrame(max_contour);			
			bounding_rect = boundingRect(max_contour);			

			if (show_rectangle) {
				rectangle(frame, bounding_rect, Scalar(255, 255, 0), 2);
				Point2f pkt = Point2f(bounding_rect.x, bounding_rect.y);
				string wspl = to_string(bounding_rect.x) + ", " + to_string(bounding_rect.y);
				putText(frame, wspl, pkt, FONT_HERSHEY_COMPLEX_SMALL, 2, (255, 255, 255), 2, LINE_AA);
			}

			is_detected = true;
		}
	}

	return is_detected;
}

Mat prepareToDetect(Mat frame, Mat frame_copy, VideoCapture video)
{
	Mat blurr_diff, diff, gray_frame1, gray_frame2, thresh;
	
	cvtColor(frame, gray_frame2, COLOR_BGR2GRAY);
	cvtColor(frame_copy, gray_frame1, COLOR_BGR2GRAY);

	absdiff(gray_frame1, gray_frame2, diff);
	GaussianBlur(diff, blurr_diff, Size(5, 5), 20);
	threshold(blurr_diff, thresh, 20.0, 255.0, THRESH_BINARY);

	return thresh;
}

bool isNextCar(bool is_car_detected, bool &counter_started)
{
	if (is_car_detected == true && counter_started == true) {
		counter_started = false;
		return true;
	}

	if (is_car_detected == false) {
		counter_started = true;
	}

	return false;
}

void makeScreenShoot(Mat frame, int number)
{
	string file_name = to_string(number) + ".jpg";
	imwrite(file_name, frame);
}

void detectCars(string filename)
{
	namedWindow("settings");

	bool is_car_detected;
	bool counter_started = true;
	int stop = 0;
	int car_counter = 0;
	int frames_with_car = 0;
	int show_rectangle = 1;
	int make_screenshots = 1;
	Mat frame, frame_copy;
	VideoCapture video(filename);

	createTrackbar("Pokaz pojazdy", "settings", &show_rectangle, 1);
	createTrackbar("Stop", "settings", &stop, 1);
	createTrackbar("Rob screenshoty", "settings", &make_screenshots, 1);

	video.read(frame);

	while (waitKey(15) != 27)
	{
		frame_copy = frame.clone();

		if (!video.read(frame)) {
			break;
		}

		Mat thresh = prepareToDetect(frame, frame_copy, video);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		findContours(thresh(cv::Rect(10, DETECT_BOX_SHIFT_Y, 1850, 600)), contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

		is_car_detected = detectAndShowCntours(contours, frame, (bool) show_rectangle);

		if (is_car_detected) {
			frames_with_car++;
		}	

		if (isNextCar(is_car_detected, counter_started)) {
			car_counter++;
			if (make_screenshots) {
				makeScreenShoot(frame, car_counter);
			}
		}
				
		putText(frame, "Licznik samochodow:" + to_string(car_counter), Point2f(30,30), FONT_HERSHEY_COMPLEX_SMALL, 2, (145, 145, 145), 2, LINE_AA);
		putText(frame, "Klatki z samochodami:" + to_string(frames_with_car), Point2f(30,60), FONT_HERSHEY_COMPLEX_SMALL, 2, (145, 145, 145), 2, LINE_AA);

		if (stop != 1) {
			imshow("window", frame);
		}
	}
	video.release();
	destroyAllWindows();
}

int main(void)
{
	cv::Mat frame = cv::Mat(200, 700, CV_8UC3);
	string file_path = "";

	cv::namedWindow(WINDOW_NAME);
	cvui::init(WINDOW_NAME);

	while (true) {
		frame = Scalar(49, 52, 49);
		if (file_path != "") {
			if (cvui::button(frame, 10, 10, "Wykrywaj pojazdy")) {
				detectCars(file_path);
			}
		}
		if (cvui::button(frame, 150, 10, "Zamknij program")) {
			destroyAllWindows();
			break;
		}
		cvui::printf(frame, 10, 50, "Aby pokazal sie przycisk do startu nalezy podac sciezke do pliku w drugim oknie");
		cvui::printf(frame, 10, 90, "Niestety w programie musialem uzyc starych 'trackbarow'.");
		cvui::printf(frame, 10, 110, "blad: 'https://github.com/Dovyski/cvui/issues/70'");
		cvui::printf(frame, 10, 150, "Mikolaj Bronk");
		cvui::update();
		cv::imshow(WINDOW_NAME, frame);

		if (file_path == "") {
			cout << "Podaj sciezke do pliku" << endl;
			cin >> file_path;
			VideoCapture test_video;

			if (!test_video.open(file_path)) {
				cout << "Zly plik! " << endl;
				file_path = "";
			} else {
				cout << "Plik zaimportowano pomyslnie! Kontynuuj w drugim oknie" << endl;
			}

			test_video.release();
		}

		if (cv::waitKey(20) == 27) {
			break;
		}
	}
	return 0;
}