// main for async video stream project

//local headers
#include "cv_vid_background.h"
#include "histogram_median_algo.h"
#include "project_dir_config.h"
#include "triframe_median_algo.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <iostream>
#include <memory>
#include <thread>		//for std::thread::hardware_concurrency()
#include <vector>


// command line parameters (compatible with cv::CommandLineParser)
const char* g_commandline_params = 
	"{ help h      |    | Print usage }"
	"{ vid         |    | Video name (with extension) }"
	"{ vid_path    |    | Full path for video }"
	"{ frame_limit | -1 | Max number of frames to analyze }"
	"{ max_threads |  8 | Max number of threads to use for analyzing the video }";


int main(int argc, char* argv[])
{
	cv::CommandLineParser cl_args{argc, argv, g_commandline_params};

	// print help message if necessary
	if (cl_args.has("help"))
	{
		// prints contents of command line parameters
		cl_args.printMessage();
	}

	// open video file (move assigns from temporary constructed by string, presumably)
	cv::VideoCapture vid;
	if (cl_args.get<cv::String>("vid") != "")
		vid = std::string{config::videos_dir} + cl_args.get<cv::String>("vid");
	else if (cl_args.get<cv::String>("vid_path") != "")
		vid = cl_args.get<cv::String>("vid_path");

	if (!vid.isOpened())
	{
		std::cerr << "Video file not detected: " << cl_args.get<cv::String>("vid") << '\n';

		return 0;
	}

	// print info about the video
	int total_frames{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_COUNT))};
	std::cout << "Frames: " << total_frames <<
				  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT) <<
				  "; FPS: " << vid.get(cv::CAP_PROP_FPS) <<
		 "; Pixel format: " << get_fourcc_code_str(vid.get(cv::CAP_PROP_FOURCC)) << '\n';

	// get frame limit
	int frame_limit{cl_args.get<int>("frame_limit")};
	if (frame_limit < 0 || frame_limit > total_frames)
		frame_limit = total_frames;

	// get number of worker threads to use (subtract one for the main thread)
	int worker_threads{cl_args.get<int>("max_threads")};
	const auto processor_cores{std::thread::hardware_concurrency()};
	if (worker_threads <= 0)
		worker_threads = 1;
	else if (worker_threads >= 2*processor_cores)
		worker_threads = 2*processor_cores - 1;
	else if (worker_threads > 1)
		worker_threads -= 1;

	// get the background of the video
	using MedianAlgo = HistogramMedianAlgo16;

	std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
	empty_packs.resize(worker_threads, TokenProcessorPack<MedianAlgo>{});

	CvVidBackground<MedianAlgo> get_background_process{empty_packs, vid, frame_limit, 0, 0, worker_threads, 3, 3};

	std::unique_ptr<cv::Mat> background_frame{get_background_process.Run()};

	// display the final median image
	if (background_frame && background_frame->data && !background_frame->empty())
		cv::imshow("Median Frame", *background_frame);
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";

	// wait for a keypress before ending
	int keypress{cv::waitKey()};

	return 0;
}















