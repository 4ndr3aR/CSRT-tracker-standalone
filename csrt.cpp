#include <tracker.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>
#include "samples_utility.hpp"

#include "opencv2/core/ocl.hpp"

using namespace std;
using namespace cv;

void splitstring (const std::string &str, char separator, std::vector<std::string> &vec)
{
        std::string word;
        std::stringstream stream (str);

        vec.clear();

        while (std::getline (stream, word, separator))
        {
                vec.push_back (word);
        }
}


int main(int argc, char** argv)
{
    bool show_images   = false;
    bool save_video    = true;
    bool resize_imshow = false;
    bool debug         = true;

    // show help
    if (argc<2) {
        cout <<
            " Usage: example_tracking_csrt <video_name>\n"
            " examples:\n"
            " example_tracking_csrt Bolt/img/%04.jpg\n"
            " example_tracking_csrt Bolt/img/%04.jpg Bolt/grouondtruth.txt\n"
            " example_tracking_csrt faceocc2.webm\n"
            << endl;
        return 0;
    }

    // create the tracker
    Ptr<TrackerCSRT> tracker = TrackerCSRT::create();

    // const char* param_file_path = "/home/amuhic/Workspace/3_dip/params.yml";
    // FileStorage fs(params_file_path, FileStorage::WRITE);
    // tracker->write(fs);
    // FileStorage fs(param_file_path, FileStorage::READ);
    // tracker->read( fs.root());

    // set input video
    std::string video = argv[1];
    VideoCapture cap(video);
    // and read first frame
    Mat frame;
    cap >> frame;

    // target bounding box
    Rect2d roi;
    if (argc > 3) {
        // read first line of ground-truth file
        std::string groundtruthPath = argv[3];
        std::ifstream gtIfstream(groundtruthPath.c_str());
        std::string gtLine;
        getline(gtIfstream, gtLine);
        gtIfstream.close();

        // parse the line by elements
        std::stringstream gtStream(gtLine);
        std::string element;
        std::vector<int> elements;
        while (std::getline(gtStream, element, ','))
        {
            elements.push_back(cvRound(std::atof(element.c_str())));
        }

        if (elements.size() == 4) {
            // ground-truth is rectangle
            roi = cv::Rect(elements[0], elements[1], elements[2], elements[3]);
        }
        else if (elements.size() == 8) {
            // ground-truth is polygon
            int xMin = cvRound(min(elements[0], min(elements[2], min(elements[4], elements[6]))));
            int yMin = cvRound(min(elements[1], min(elements[3], min(elements[5], elements[7]))));
            int xMax = cvRound(max(elements[0], max(elements[2], max(elements[4], elements[6]))));
            int yMax = cvRound(max(elements[1], max(elements[3], max(elements[5], elements[7]))));
            roi = cv::Rect(xMin, yMin, xMax - xMin, yMax - yMin);

            // create mask from polygon and set it to the tracker
            cv::Rect aaRect = cv::Rect(xMin, yMin, xMax - xMin, yMax - yMin);
            cout << aaRect.size() << endl;
            Mat mask = Mat::zeros(aaRect.size(), CV_8UC1);
            const int n = 4;
            std::vector<cv::Point> poly_points(n);
            //Translate x and y to rects start position
            int sx = aaRect.x;
            int sy = aaRect.y;
            for (int i = 0; i < n; ++i) {
                poly_points[i] = Point(elements[2 * i] - sx, elements[2 * i + 1] - sy);
            }
            cv::fillConvexPoly(mask, poly_points, Scalar(1.0), 8);
            mask.convertTo(mask, CV_32FC1);
            tracker->setInitialMask(mask);
        }
        else {
            cout << "Number of ground-truth elements is not 4 or 8." << endl;
        }

    }
    else {
	if (argc == 2)
	{
		// second argument is not given - user selects target
	        roi = selectROI("tracker", frame, true, false);
		// ./csrt ../blob-tracking/src/media/boat-test-15secs-Drone-flight-207_-flight-over-the-ocean-off-Newport-Beach.mp4
		// Roi: [42 x 42 from (547, 469)]
		cout << "Roi: " << roi << endl;
	}
	else
	{
		// second argument is a ROI, read it... (four numbers: x, y, w, h - let's write them in this way: x,y,w,h)
		// ./csrt ../blob-tracking/src/media/boat-test-15secs-Drone-flight-207_-flight-over-the-ocean-off-Newport-Beach.mp4 547,469,42,42

		std::vector<std::string> words;
		splitstring(argv[2], ',', words);
		cout << words[0] << " " << words[1] << " " << words[2] << " " << words[3] << endl;
		roi.x = atoi(words[0].c_str());
		roi.y = atoi(words[1].c_str());
		roi.width = atoi(words[2].c_str());
		roi.height = atoi(words[3].c_str());

		cout << "Roi: " << roi << endl;
	}
    }

    //quit if ROI was not selected
    if (roi.width == 0 || roi.height == 0)
        return 0;

    // initialize the tracker
    int64 t1 = cv::getTickCount();
    tracker->init(frame, roi);
    int64 t2 = cv::getTickCount();
    int64 tick_counter = t2 - t1;

    Mat imshow_mat;

    int frame_width  = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    VideoWriter video_out("/tmp/out.avi", CV_FOURCC('X','2','6','4') , 30, Size(frame_width, frame_height), true);

    // do the tracking
    printf("Start the tracking process, press ESC to quit.\n");
    int frame_idx = 1;
    for (;;) {
        // get frame from the video
        cap >> frame;

        // stop the program if no more images
        if (frame.rows == 0 || frame.cols == 0)
            break;

        // update the tracking result
        t1 = cv::getTickCount();
        bool isfound = tracker->update(frame, roi);
        t2 = cv::getTickCount();
        tick_counter += t2 - t1;
        frame_idx++;

	if (debug && frame_idx % 100 == 0)
		cout << "Processed frame: " << frame_idx << endl;

        if (!isfound) {
            cout << "The target has been lost...\n";
            waitKey(0);
            return 0;
        }

	if (show_images)
	{
		// draw the tracked object and show the image
		rectangle(frame, roi, Scalar(255, 0, 0), 5, 1);

		if (resize_imshow)
		{
			resize(frame, imshow_mat, Size(320, 240));
			imshow("tracker", imshow_mat);
		}
		else
			imshow("tracker", frame);
	}
	if (save_video)
	{
		// draw the tracked object and show the image
		rectangle(frame, roi, Scalar(255, 0, 0), 5, 1);
		video_out.write(frame);
	}

        //quit on ESC button
        if (waitKey(1) == 27)break;
    }

    cout << "Elapsed sec: " << static_cast<double>(tick_counter) / cv::getTickFrequency() << endl;
    cout << "FPS: " << ((double)(frame_idx)) / (static_cast<double>(tick_counter) / cv::getTickFrequency()) << endl;
}
