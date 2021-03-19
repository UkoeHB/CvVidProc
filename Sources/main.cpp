// main for async video stream project

//local headers
#include "cv_vid_bg_helpers.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "project_config.h"
#include "rand_tests.h"
#include "ts_interval_timer.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>

//standard headers
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>		//for std::thread::hardware_concurrency()


// command line parameters (compatible with cv::CommandLineParser)
const char* g_commandline_params = 
	"{ help h           |         | Print usage }"
	"{ vid              |         | Video name (with extension) }"
	"{ vid_path         |         | Full path for video }"
	"{ max_threads      |    -1   | Max number of threads to use for analyzing the video }"
	"{ grayscale        |  false  | Treat the video as grayscale [optimization] (true/false) }"
	"{ vid_is_grayscale |  false  | Video is already grayscale [optimization] (true/false) }"
	"{ bg_algo          |   hist  | Algorithm for getting background image (hist/tri) }"
	"{ bg_frame_lim     |    -1   | Max number of frames to analyze for background image }"
	"{ timer_report     |   true  | Collect timings for background processing and report them }";

int GetAdditionalThreads(int min_threads, int extra_threads, int max_threads)
{
	if (extra_threads < 0)
		extra_threads = 0;

	if (min_threads < 0)
		min_threads = 0;

	const auto supported_thread_count{std::thread::hardware_concurrency()};

	// note: when supported thread count fails, don't modify max_threads unless necessary
	if (max_threads < 1 || (supported_thread_count && max_threads > (supported_thread_count + extra_threads)))
		max_threads = supported_thread_count + extra_threads;

	if (max_threads > min_threads)
		return max_threads - min_threads;
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

	// get max number of threads allowed
	pack.max_threads = cl_args.get<int>("max_threads");

	// get grayscale setting
	pack.grayscale = cl_args.get<bool>("grayscale");

	// get vid_is_grayscale setting
	pack.vid_is_grayscale = cl_args.get<bool>("vid_is_grayscale");

	// get background algorithm
	pack.bg_algo = cl_args.get<cv::String>("bg_algo");

	// get frame limit
	pack.bg_frame_lim = static_cast<long long>(cl_args.get<double>("bg_frame_lim"));

	// get timer report setting
	pack.print_timing_report = cl_args.get<bool>("timer_report");

	return pack;
}

VidBgPack vidbgpack_from_clpack(const CommandLinePack &cl_pack)
{
	return VidBgPack{
		cl_pack.vid_path,
		cl_pack.bg_algo,
		cl_pack.max_threads,
		cl_pack.bg_frame_lim,
		cl_pack.grayscale,
		cl_pack.vid_is_grayscale,
		0,
		0,
		0,
		0,
		10,
		cl_pack.print_timing_report
	};
}

int main(int argc, char* argv[])
{
	// obtain command line settings
	cv::CommandLineParser cl_args{argc, argv, g_commandline_params};
	CommandLinePack cl_pack{HandleCLArgs(cl_args)};

	// time the background algo
	TSIntervalTimer timer{};
	auto start_time{timer.GetTime()};

	// get the background of the video
	cv::Mat background_frame{GetVideoBackground(vidbgpack_from_clpack(cl_pack))};

	// end the timer and print results
	timer.AddInterval(start_time);
	auto timer_report{timer.GetReport<std::chrono::milliseconds>()};
	auto interval_ms{timer_report.total_time.count()};
	auto interval_s_float{static_cast<double>(interval_ms/1000.0)};
	std::cout << "Background obtained in: " << interval_s_float << " seconds\n";

	// display the final median image
	if (background_frame.data && !background_frame.empty())
	{
		cv::imshow("Median Frame", background_frame);

		// wait for a keypress before ending
		int keypress{cv::waitKey()};
	}
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";


	//rand_tests::test_bubblehighlighting(background_frame, cl_pack, true);
	//rand_tests::test_embedded_python();
	//rand_tests::test_timing_numpyconverter(2000, true);
	//rand_tests::test_exception_assert();

	rand_tests::demo_trackbubbles(cl_pack, background_frame);

	return 0;
}















