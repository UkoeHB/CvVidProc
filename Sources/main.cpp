// main for async video stream project

//local headers
#include "cv_vid_bg_helpers.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "project_dir_config.h"
#include "ts_interval_timer.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

//standard headers
#include <chrono>
#include <iostream>
#include <thread>		//for std::thread::hardware_concurrency()


namespace py = pybind11;

// command line parameters (compatible with cv::CommandLineParser)
const char* g_commandline_params = 
	"{ help h           |         | Print usage }"
	"{ vid              |         | Video name (with extension) }"
	"{ vid_path         |         | Full path for video }"
	"{ max_threads      |   100   | Max number of threads to use for analyzing the video }"
	"{ grayscale        |  false  | Treat the video as grayscale [optimization] (true/false) }"
	"{ vid_is_grayscale |  false  | Video is already grayscale [optimization] (true/false) }"
	"{ bg_algo          |   hist  | Algorithm for getting background image (hist/tri) }"
	"{ bg_frame_lim     |    -1   | Max number of frames to analyze for background image }"
	"{ timer_report     |   true  | Collect timings for background processing and report them }";

int WorkerThreadsFromMax(int max_threads)
{
	const auto supported_thread_count{std::thread::hardware_concurrency()};

	if (max_threads < 1)
		max_threads = supported_thread_count;

	// check if hardware_concurrency() actually returned a value
	if (supported_thread_count > 0 && max_threads >= supported_thread_count)
		return supported_thread_count - (supported_thread_count > 1 ? 1 : 0);
	else if (max_threads > 1)
		return max_threads - 1;
	else
		return 1;
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

VidBgPack vidbgpack_from_clpack(const CommandLinePack &cl_pack, const int threads)
{
	return VidBgPack{
			cl_pack.vid_path,
			cl_pack.bg_algo,
			threads,
			cl_pack.bg_frame_lim,
			cl_pack.grayscale,
			cl_pack.vid_is_grayscale,
			0,
			0,
			0,
			0,
			0,
			0,
			200,
			200,
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
	cv::Mat background_frame{GetVideoBackground(vidbgpack_from_clpack(cl_pack, cl_pack.worker_threads))};

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

	/*
	// test bubble highlighting
	// for test video 'bubble_15000fps.mp4'

	// set parameters
	TokenProcessorPack<HighlightBubblesAlgo> bubbles_pack{
		background_frame,
		cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, cv::Size{4, 4}),
		14,
		7,
		16,
		20,
		20,
		5
	};

	// create algo object for processing frames (highlighting bubbles)
	HighlightBubblesAlgo highlight_bubbles{bubbles_pack};

	// get 10th frame of video
	cv::VideoCapture vid{cl_pack.vid_path};
	vid.set(cv::CAP_PROP_POS_FRAMES, 10);

	cv::Mat vid_frame{};
	vid >> vid_frame;

	// grayscale (if vid is grayscale)
	cv::Mat modified_frame{};
	cv::extractChannel(vid_frame, modified_frame, 0);

	// convert format
	std::unique_ptr<cv::Mat> frame{std::make_unique<cv::Mat>(modified_frame)};

	// insert to algo (processes the frame)
	highlight_bubbles.Insert(std::move(frame));

	// get the processed result out
	frame = highlight_bubbles.TryGetResult();

	// display the highlighted bubbles
	if (frame && frame->data && !frame->empty())
	{
		cv::imshow("Highlighted Bubbles", *frame);

		// wait for a keypress before ending
		int keypress{cv::waitKey()};
	}
	else
		std::cerr << "Bubbles frame created was malformed, unexpectedly!\n";
	*/

	// test assign_bubbles
	// create Python interpreter
	py::scoped_interpreter guard{};

	py::module_ sys = py::module_::import("sys");
	py::print(sys.attr("path"));

	return 0;
}















