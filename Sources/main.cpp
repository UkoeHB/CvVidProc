// main for async video stream project

//local headers
#include "cv_vid_bg_helpers.h"
#include "main.h"
#include "project_dir_config.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <iostream>
#include <thread>		//for std::thread::hardware_concurrency()


// command line parameters (compatible with cv::CommandLineParser)
const char* g_commandline_params = 
	"{ help h       |         | Print usage }"
	"{ vid          |         | Video name (with extension) }"
	"{ vid_path     |         | Full path for video }"
	"{ max_threads  |     8   | Max number of threads to use for analyzing the video }"
	"{ grayscale    |  false  | Treat the video as grayscale (true/false) }"
	"{ bg_algo      |   hist  | Algorithm for getting background image (hist/tri) }"
	"{ bg_frame_lim |    -1   | Max number of frames to analyze for background image }";

int WorkerThreadsFromMax(int max_threads)
{
	const auto supported_thread_count{std::thread::hardware_concurrency()};

	if (max_threads <= 0)
		return max_threads = 1;
	// check if hardware_concurrency() actually returned a value
	else if (supported_thread_count > 0 && max_threads >= supported_thread_count)
		return max_threads = supported_thread_count - (supported_thread_count > 1 ? 1 : 0);
	else if (max_threads > 1)
		return max_threads -= 1;
	else
		return 0;
}

CommandLinePack HandleCLArgs(cv::CommandLineParser &cl_args)
{
	CommandLinePack pack{};

	// print help message if necessary
	if (cl_args.has("help"))
	{
		// prints contents of command line parameters
		cl_args.printMessage();
	}

	// get path to video
	if (cl_args.get<cv::String>("vid") != "")
		pack.vid_path = std::string{config::videos_dir} + cl_args.get<cv::String>("vid");
	else if (cl_args.get<cv::String>("vid_path") != "")
		pack.vid_path = cl_args.get<cv::String>("vid_path");

	// get number of worker threads to use (subtract one for the main thread)
	// note: min threads is 2 (1 for main thread, 1 for worker)
	pack.worker_threads = WorkerThreadsFromMax(cl_args.get<int>("max_threads"));

	// get grayscale setting
	pack.grayscale = cl_args.get<bool>("grayscale");

	// get background algorithm
	pack.bg_algo = cl_args.get<cv::String>("bg_algo");

	// get frame limit
	pack.bg_frame_lim = static_cast<long long>(cl_args.get<double>("bg_frame_lim"));

	return pack;
}

VidBgPack vidbgpack_from_clpack(const CommandLinePack &cl_pack, const int threads)
{
	return VidBgPack{
			cl_pack.vid_path,
			cl_pack.bg_algo,
			threads,
			cl_pack.bg_frame_lim,
			cl_pack.grayscale,
			0,
			0,
			200,
			200
		};
}

int main(int argc, char* argv[])
{
	// obtain command line settings
	cv::CommandLineParser cl_args{argc, argv, g_commandline_params};
	CommandLinePack cl_pack{HandleCLArgs(cl_args)};

	// get the background of the video
	cv::Mat background_frame{GetVideoBackground(vidbgpack_from_clpack(cl_pack, cl_pack.worker_threads))};

	// display the final median image
	if (background_frame.data && !background_frame.empty())
		cv::imshow("Median Frame", background_frame);
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";

	// wait for a keypress before ending
	int keypress{cv::waitKey()};

	return 0;
}















