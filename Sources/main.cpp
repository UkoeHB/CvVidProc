// test file for async video stream project

//local headers
#include "async_token_set_queue.h"
#include "background_processor.h"
#include "bgprocessor_triframemedian.h"
#include "cv_util.h"
#include "project_dir_config.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <array>
#include <iostream>
#include <future>
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

// get median frame from input video (copy constructed VideoCapture so original is not affected)
// uses dependency injection via template argument to obtain the background processor algo
template <typename BgProcessorT>
cv::Mat get_background(cv::VideoCapture vid, const int frame_limit, const int worker_threads, const BgProcessorPack<BgProcessorT> &processor_pack)
{
	if (!vid.isOpened())
		return cv::Mat{};

	// create token queue for holding frame fragments in need of processing
	// must limit max queue size strictly to avoid memory problems, since video frames are large and there are a lot of them
	AsyncTokenSetQueue<std::unique_ptr<cv::Mat>> token_queue{worker_threads, 1};

	// prepare result fragments
	std::vector<std::future<cv::Mat>> fragment_result_futures{};
	fragment_result_futures.reserve(worker_threads);

	// launch worker threads for processing fragments
	for (int queue_index{0}; queue_index < worker_threads; queue_index++)
	{
		fragment_result_futures.emplace_back(std::async(std::launch::async,
				[&token_queue, &processor_pack, queue_index]() -> cv::Mat
				{
					// relies on template dependency injection to decide the processor algorithm
					BackgroundProcessor<BgProcessorT, cv::Mat> worker_processor{processor_pack};
					std::unique_ptr<cv::Mat> mat_fragment_shuttle{};

					// get tokens asynchronously until the queue shuts down (and is empty)
					while(token_queue.GetToken(mat_fragment_shuttle, queue_index))
					{
						worker_processor.Insert(std::move(mat_fragment_shuttle));
					}

					return worker_processor.GetResult();
				}
			));
	}

	// cycle through video frames
	cv::Mat frame{};
	int num_frames{0};

	while (num_frames++ < frame_limit)
	{
		// get next frame from video
		vid >> frame;

		// leave if reached the end of the video or frame is corrupted
		if (!frame.data || frame.empty())
			break;

		// break frame into chunks
		std::vector<std::unique_ptr<cv::Mat>> frame_chunks{};
		if (!cv_mat_to_chunks(frame, frame_chunks, 1, worker_threads))
			std::cerr << "Breaking frame (" << num_frames << ") into chunks failed unexpectedly!\n";

		// pass chunks to processor queue
		token_queue.InsertAll(std::move(frame_chunks));
	}

	// inform queue that no more tokens are being added
	token_queue.ShutDown();

	// release the video since this 'videocapture' can't be used any more
	vid.release();

	// collect all the result fragments
	std::vector<cv::Mat> collect_results{};

	for (int queue_index{0}; queue_index < worker_threads; queue_index++)
	{
		// get result from std::future object
		collect_results.emplace_back(fragment_result_futures[queue_index].get());
	}

	// combine result fragments
	cv::Mat final_background{};
	if (!cv_mat_from_chunks(final_background, collect_results, 1, worker_threads))
		std::cerr << "Combining final results into background image failed unexpectedly!\n";

	return final_background;
}

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

	// get number of threads to use
	int worker_threads{cl_args.get<int>("max_threads")};
	const auto processor_cores{std::thread::hardware_concurrency()};
	if (worker_threads <= 0)
		worker_threads = 1;
	else if (worker_threads > 2*processor_cores)
		worker_threads = 2*processor_cores - 1;
	else if (worker_threads > 1)
		worker_threads -= 1;

	// get the background of the video
	cv::Mat background_frame{get_background<TriframeMedian>(vid, frame_limit, worker_threads, BgProcessorPack<TriframeMedian>{})};

	// display the final median image
	if (background_frame.data && !background_frame.empty())
		cv::imshow("Median Frame", background_frame);
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";

	// wait for a keypress before ending
	int keypress{cv::waitKey()};

	return 0;
}















