// main for async video stream project

//local headers
#include "cv_vid_background.h"
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
	"{ bg_algo      |   hist  | Algorithm for getting background image (hist/tri) }"
	"{ bg_frame_lim |    -1   | Max number of frames to analyze for background image }"
	"{ grayscale    |  false  | Treat the video as grayscale (true/false) }";

struct CommandLinePack
{
	cv::String video_path{};
	unsigned long frame_limit{};
	int worker_threads{};
	bool use_grayscale{};
};

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

	// get frame limit
	pack.frame_limit = cl_args.get<int>("bg_frame_lim");

	// get number of worker threads to use (subtract one for the main thread)
	int worker_threads{cl_args.get<int>("max_threads")};
	const auto processor_cores{std::thread::hardware_concurrency()};
	if (worker_threads <= 0)
		worker_threads = 1;
	else if (worker_threads >= 2*processor_cores)
		worker_threads = 2*processor_cores - 1;
	else if (worker_threads > 1)
		worker_threads -= 1;

	pack.worker_threads = worker_threads;

	// get grayscale setting
	pack.grayscale = cl_args.get<bool>("grayscale");
}

std::unique_ptr<cv::Mat> GetVideoBackground(cv::VideoCapture &vid, const CommandLinePack &cl_pack)
{
	// algo is user-specified
	switch (cl_pack.bg_algo)
	{
		case "hist" :
			{
				// use cheapest histogram algorithm
				if (cl_pack.frame_limit <= static_cast<unsigned long>(static_cast<unsigned char>(-1)))
				{
					using MedianAlgo = HistogramMedianAlgo8;

					std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
					empty_packs.resize(cl_pack.worker_threads, TokenProcessorPack<MedianAlgo>{});

					return VidBackgroundWithAlgo<MedianAlgo>(vid, cl_pack, empty_packs);
				}
				else if (cl_pack.frame_limit <= static_cast<unsigned long>(static_cast<std::uint16_t>(-1)))
				{
					using MedianAlgo = HistogramMedianAlgo16;

					std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
					empty_packs.resize(cl_pack.worker_threads, TokenProcessorPack<MedianAlgo>{});

					return VidBackgroundWithAlgo<MedianAlgo>(vid, cl_pack, empty_packs);
				}
				else if (cl_pack.frame_limit <= static_cast<unsigned long>(static_cast<std::uint32_t>(-1)))
				{
					using MedianAlgo = HistogramMedianAlgo32;

					std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
					empty_packs.resize(cl_pack.worker_threads, TokenProcessorPack<MedianAlgo>{});

					return VidBackgroundWithAlgo<MedianAlgo>(vid, cl_pack, empty_packs);
				}
				else
				{
					std::cerr << "warning, video appears to have over 2^32 frames! (" << cl_pack.frame_limit << ") is way too many!\n";
				}
			}

		case "tri" :
			{
				using MedianAlgo = TriframeMedianAlgo;

				std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
				empty_packs.resize(cl_pack.worker_threads, TokenProcessorPack<MedianAlgo>{});

				return VidBackgroundWithAlgo<MedianAlgo>(vid, cl_pack, empty_packs);
			}

		default :
		{
			std::cerr << "Unknown background algorithm detected: " << cl_pack.bg_algo << '\n';
		}
	};

	return std::unique_ptr<cv::Mat>{};
}

template <typename MedianAlgo>
std::unique_ptr<cv::Mat> VidBackgroundWithAlgo(cv::VideoCapture &vid, const CommandLinePack &cl_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs)
{
	CvVidBackground<MedianAlgo> get_background_process{processor_packs, vid, cl_pack.frame_limit, 0, 0, cl_pack.grayscale, cl_pack.worker_threads, 3, 3};

	return get_background_process.Run();
}


int main(int argc, char* argv[])
{
	// obtain command line settings
	cv::CommandLineParser cl_args{argc, argv, g_commandline_params};
	CommandLinePack cl_pack{HandleCLArgs(cl_args)};

	// open video file (move assigns from temporary constructed by string, presumably)
	cv::VideoCapture vid{cl_pack.video_path};

	if (!vid.isOpened())
	{
		std::cerr << "Video file not detected: " << cl_pack.video_path << '\n';

		return 0;
	}

	// print info about the video
	unsigned long total_frames{static_cast<unsigned long>(vid.get(cv::CAP_PROP_FRAME_COUNT))};
	std::cout << "Frames: " << total_frames <<
				  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT) <<
				  "; FPS: " << vid.get(cv::CAP_PROP_FPS) << '\n';

	// clean up frame limit
	if (cl_pack.frame_limit < 0 || cl_pack.frame_limit > total_frames)
		cl_pack.frame_limit = total_frames;

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















