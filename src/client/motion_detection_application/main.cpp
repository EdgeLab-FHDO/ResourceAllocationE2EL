#include <iostream>
#include <sstream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <chrono>

using namespace cv;
using namespace std;
using namespace std::chrono;

int main(int argc, char* argv[])
{
	//create Background Subtractor objects
	Ptr<BackgroundSubtractor> pBackSub;
	//pBackSub = createBackgroundSubtractorMOG2(500);
	//pBackSub = createBackgroundSubtractorKNN();

	VideoCapture capture("1.mp4");
	if (!capture.isOpened()) {
		//error in opening the video input
		cerr << "Unable to open camera" << endl;
		return 0;
	}

	double fps = capture.get(CAP_PROP_FPS);
	double number_of_frames = capture.get(CAP_PROP_FRAME_COUNT);
	double video_duration = (number_of_frames / fps) * 1000;

	//capture.set(CAP_PROP_FRAME_WIDTH, 320);
	//capture.set(CAP_PROP_FRAME_HEIGHT, 240);

	Mat frame, previous_frame, modified_frame, previous_modified_frame, difference_frame, fgMask, dilated_frame, final_frame, previous_frame_resized, frame_resized;
	Size size(256, 144);

	capture >> previous_frame;
	resize(previous_frame, previous_frame_resized, size);
	cvtColor(previous_frame_resized, previous_modified_frame, COLOR_BGR2GRAY);
	GaussianBlur(previous_modified_frame, previous_modified_frame, Size(11, 11), 0);

	time_point<system_clock, milliseconds> start_tp = time_point_cast<milliseconds, system_clock>(system_clock::now());
	time_point<system_clock, milliseconds> current_tp = start_tp;

	double current_video_time = 0;

	while (true) {

		current_tp = current_tp + milliseconds(200);
		this_thread::sleep_until(current_tp);
		current_tp = time_point_cast<milliseconds, system_clock>(system_clock::now());

		current_video_time = (current_tp - start_tp).count() % ((int)video_duration);

		capture.set(CAP_PROP_POS_MSEC, current_video_time);

		capture >> frame;
		if (frame.empty())
		{
			continue;
		}

		resize(frame, frame_resized, size);
		//update the background model
		//pBackSub->apply(frame, fgMask);

		final_frame = frame_resized.clone();

		cvtColor(frame_resized, modified_frame, COLOR_BGR2GRAY);
		GaussianBlur(modified_frame, modified_frame, Size(11, 11), 0);

		absdiff(modified_frame, previous_modified_frame, difference_frame);
		//cvtColor(difference_frame, difference_frame, COLOR_BGR2GRAY);

		threshold(difference_frame, fgMask, 12, 255, THRESH_BINARY);

		erode(fgMask, dilated_frame, Mat(), Point(-1, -1), 2);
		dilate(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 2);

		dilate(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 12);
		erode(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 12);


		RNG rng(12345);
		vector<vector<Point> > contours;
		vector<Point> contours_poly;
		Rect boundRect;
		Point2f center;
		float radius;
		vector<Vec4i> hierarchy;
		findContours(dilated_frame, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
		Mat contours_frame = Mat::zeros(dilated_frame.size(), CV_8UC3);
		for (size_t i = 0; i < contours.size(); i++)
		{
			if (contours.at(i).size() > 15)
			{
				Scalar color = Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));

				approxPolyDP(Mat(contours[i]), contours_poly, 3, true);
				boundRect = boundingRect(Mat(contours_poly));
				minEnclosingCircle((Mat)contours_poly, center, radius);

				drawContours(contours_frame, contours, (int)i, color, 2, LINE_8, hierarchy, 0);

				rectangle(final_frame, boundRect.tl(), boundRect.br(), color, 2, 8, 0);
				//circle(final_frame, center, (int)radius, color, 2, 8, 0);
			}
		}


		//imshow("Frame", frame_resized);
		//imshow("Modified Frame", modified_frame);
		//imshow("Difference Frame", difference_frame);
		//imshow("FG Mask", fgMask);
		//imshow("Dilated Frame", dilated_frame);
		//imshow("Contours", contours_frame);
		imshow("Final Frame", final_frame);

		//previous_frame = frame.clone();

		//get the input from the keyboard
		int keyboard = waitKey(1);
		if (keyboard == 'q' || keyboard == 27)
		{
			capture >> previous_frame;
			resize(previous_frame, previous_frame_resized, size);
			cvtColor(previous_frame_resized, previous_modified_frame, COLOR_BGR2GRAY);
			GaussianBlur(previous_modified_frame, previous_modified_frame, Size(11, 11), 0);
			//break;
		}
	}
	return 0;
}