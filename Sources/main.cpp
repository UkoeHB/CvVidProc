// main for async video stream project

//local headers
#include "cv_vid_frames_generator.h"
#include "cv_vid_background_consumer.h"
#include "async_token_process.h"
#include "histogram_median_algo.h"
#include "project_dir_config.h"
#include "triframe_median_algo.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>		//for std::thread::hardware_concurrency()
#include <vector>


// command line parameters (compatible with cv::CommandLineParser)
const char* g_commandline_params = 
	"{ help h       |         | Print usage }"
	"{ vid          |         | Video name (with extension) }"
	"{ vid_path     |         | Full path for video }"
	"{ max_threads  |     8   | Max number of threads to use for analyzing the video }"
	"{ grayscale    |  false  | Treat the video as grayscale (true/false) }"
	"{ bg_algo      |   hist  | Algorithm for getting background image (hist/tri) }"
	"{ bg_frame_lim |    -1   | Max number of frames to analyze for background image }";

struct CommandLinePack
{
	cv::String vid_path{};
	int worker_threads{};
	bool grayscale{};
	long long bg_frame_lim{};
	cv::String bg_algo{};
};

/// background algorithms available
enum class BGAlgo
{
	HISTOGRAM,
	TRIFRAME,
	UNKNOWN
};

BGAlgo GetBGAlgo(const cv::String &algo)
{
	if (algo == "hist")
		return BGAlgo::HISTOGRAM;
	else if (algo == "tri")
		return BGAlgo::TRIFRAME;
	else
		return BGAlgo::UNKNOWN;
}

/// interpret the command line arguments
CommandLinePack HandleCLArgs(cv::CommandLineParser &cl_args)
{
	CommandLinePack pack{};

	// print help message if necessary
	if (cl_args.has("help"))
	{
		// prints contents of command line parameters
		cl_args.printMessage();
	}

	// open video file (move assigns from temporary constructed by string, presumably)
	cv::VideoCapture vid;
	if (cl_args.get<cv::String>("vid") != "")
		pack.vid_path = std::string{config::videos_dir} + cl_args.get<cv::String>("vid");
	else if (cl_args.get<cv::String>("vid_path") != "")
		pack.vid_path = cl_args.get<cv::String>("vid_path");

	// get number of worker threads to use (subtract one for the main thread)
	// note: min threads is 2 (1 for main thread, 1 for worker)
	int worker_threads{cl_args.get<int>("max_threads")};
	const auto supported_thread_count{std::thread::hardware_concurrency()};
	if (worker_threads <= 0)
		worker_threads = 1;
	// check if hardware_concurrency() actually returned a value
	else if (supported_thread_count > 0 && worker_threads >= supported_thread_count)
		worker_threads = supported_thread_count - (supported_thread_count > 1 ? 1 : 0);
	else if (worker_threads > 1)
		worker_threads -= 1;

	pack.worker_threads = worker_threads;

	// get grayscale setting
	pack.grayscale = cl_args.get<bool>("grayscale");

	// get background algorithm
	pack.bg_algo = cl_args.get<cv::String>("bg_algo");

	// get frame limit
	pack.bg_frame_lim = static_cast<long long>(cl_args.get<double>("bg_frame_lim"));

	return pack;
}

/// encapsulates call to async tokenized video background analysis
template <typename MedianAlgo>
std::unique_ptr<cv::Mat> VidBackgroundWithAlgo(cv::VideoCapture &vid, const CommandLinePack &cl_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs)
{
	const int horizontal_buffer_pixels{0};
	const int vertical_buffer_pixels{0};
	const int batch_size{cl_pack.worker_threads};

	// create frame generator
	auto frame_gen = std::make_shared<CvVidFramesGenerator>{vid,
		horizontal_buffer_pixels,
		vertical_buffer_pixels,
		cl_pack.bg_frame_lim,
		cl_pack.grayscale,
		batch_size};

	// create fragment consumer
	auto bg_frag_consumer = std::make_shared<CvVidBackgroundConsumer>{vid,
		horizontal_buffer_pixels,
		vertical_buffer_pixels,
		batch_size};

	// create process
	const int token_storage_limit{3};
	const int result_storage_limit{3};

	AsyncTokenProcess<MedianAlgo, cv::Mat> vid_bg_prod{batch_size,
		token_storage_limit,
		result_storage_limit,
		frame_gen,
		bg_frag_consumer};

	return vid_bg_prod.Run(st::move(processor_packs));
}

/// encapsulates call to async tokenized video background analysis using empty processor packs
template <typename MedianAlgo>
std::unique_ptr<cv::Mat> VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const CommandLinePack &cl_pack)
{
	std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
	empty_packs.resize(cl_pack.worker_threads, TokenProcessorPack<MedianAlgo>{});

	return VidBackgroundWithAlgo<MedianAlgo>(vid, cl_pack, empty_packs);
}

/// get a video background
std::unique_ptr<cv::Mat> GetVideoBackground(cv::VideoCapture &vid, const CommandLinePack &cl_pack)
{
	// algo is user-specified
	switch (GetBGAlgo(cl_pack.bg_algo))
	{
		case BGAlgo::HISTOGRAM :
		{
			// use cheapest histogram algorithm
			if (cl_pack.bg_frame_lim <= static_cast<long long>(static_cast<unsigned char>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo8>(vid, cl_pack);
			}
			else if (cl_pack.bg_frame_lim <= static_cast<long long>(static_cast<std::uint16_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo16>(vid, cl_pack);
			}
			else if (cl_pack.bg_frame_lim <= static_cast<long long>(static_cast<std::uint32_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo32>(vid, cl_pack);
			}
			else
			{
				std::cerr << "warning, video appears to have over 2^32 frames! (" << cl_pack.bg_frame_lim << ") is way too many!\n";
			}
		}

		case BGAlgo::TRIFRAME :
		{
			return VidBackgroundWithAlgoEmptyPacks<TriframeMedianAlgo>(vid, cl_pack);
		}

		default :
		{
			std::cerr << "Unknown background algorithm detected: " << cl_pack.bg_algo << '\n';
		}
	};

	return std::unique_ptr<cv::Mat>{};
}


int main(int argc, char* argv[])
{
	// obtain command line settings
	cv::CommandLineParser cl_args{argc, argv, g_commandline_params};
	CommandLinePack cl_pack{HandleCLArgs(cl_args)};

	// open video file (move assigns from temporary constructed by string, presumably)
	cv::VideoCapture vid{cl_pack.vid_path};

	if (!vid.isOpened())
	{
		std::cerr << "Video file not detected: " << cl_pack.vid_path << '\n';

		return 0;
	}

	// print info about the video
	long long total_frames{static_cast<long long>(vid.get(cv::CAP_PROP_FRAME_COUNT))};
	std::cout << "Frames: " << total_frames <<
				  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT) <<
				  "; FPS: " << vid.get(cv::CAP_PROP_FPS) << '\n';

	// clean up background algo frame limit
	if (cl_pack.bg_frame_lim < 0 || cl_pack.bg_frame_lim > total_frames)
		cl_pack.bg_frame_lim = total_frames;

	// get the background of the video
	std::unique_ptr<cv::Mat> background_frame{GetVideoBackground(vid, cl_pack)};

	// display the final median image
	if (background_frame && background_frame->data && !background_frame->empty())
		cv::imshow("Median Frame", *background_frame);
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";

	// wait for a keypress before ending
	int keypress{cv::waitKey()};

	return 0;
}















