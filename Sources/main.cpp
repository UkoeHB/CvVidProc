// test file for async video stream project

//local headers
#include "async_token_queue.h"
#include "token_processor.h"
#include "triframe_median_algo.h"
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
#include <type_traits>
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
template <typename FrameProcessorT>
cv::Mat filter_vid_for_frame(cv::VideoCapture vid, const int frame_limit, const int worker_threads, const std::vector<TokenProcessorPack<FrameProcessorT>> &processor_packs)
{
	using TP_T = TokenProcessor<FrameProcessorT, cv::Mat>;
	static_assert(std::is_base_of<TokenProcessorBase<FrameProcessorT, cv::Mat>, TP_T>::value,
			"Token processor implementation does not derive from the TokenProcessorBase!");

	if (!vid.isOpened() ||
		processor_packs.size() != worker_threads ||
		worker_threads <= 0)
		return cv::Mat{};

	// create token queues for holding frame fragments in need of processing
	// must limit max queue size strictly to avoid memory problems, since video frames are large and there are a lot of them
	std::vector<AsyncTokenQueue<std::unique_ptr<cv::Mat>>> token_queues{};
	token_queues.resize(worker_threads, AsyncTokenQueue<std::unique_ptr<cv::Mat>>{3});

	// prepare result fragments
	std::vector<std::future<cv::Mat>> fragment_result_futures{};
	fragment_result_futures.reserve(worker_threads);

	// launch worker threads for processing fragments
	for (int queue_index{0}; queue_index < worker_threads; queue_index++)
	{
		fragment_result_futures.emplace_back(std::async(std::launch::async,
				[&token_queues, &processor_packs, queue_index]() -> cv::Mat
				{
					// relies on template dependency injection to decide the processor algorithm
					TP_T worker_processor{processor_packs[queue_index]};
					std::unique_ptr<cv::Mat> mat_fragment_shuttle{};

					// get tokens asynchronously until the queue shuts down (and is empty)
					while(token_queues[queue_index].GetToken(mat_fragment_shuttle))
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

		// pass chunks to processor queues
		for (int queue_index{0}; queue_index < worker_threads; ++queue_index)
		{
			token_queues[queue_index].Insert(std::move(frame_chunks[queue_index]));
		}
	}

	// release the video since this 'videocapture' can't be used any more
	vid.release();

	// collect all the result fragments
	std::vector<cv::Mat> collect_results{};

	for (int queue_index{0}; queue_index < worker_threads; queue_index++)
	{
		// inform queues that no more tokens are being added
		token_queues[queue_index].ShutDown();

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
	std::vector<TokenProcessorPack<TriframeMedianAlgo>> empty_pack;
	empty_pack.resize(worker_threads, TokenProcessorPack<TriframeMedianAlgo>{});

	cv::Mat background_frame{filter_vid_for_frame<TriframeMedianAlgo>(vid, frame_limit, worker_threads, empty_pack)};

	// display the final median image
	if (background_frame.data && !background_frame.empty())
		cv::imshow("Median Frame", background_frame);
	else
		std::cerr << "Background frame created was malformed, unexpectedly!\n";

	// wait for a keypress before ending
	int keypress{cv::waitKey()};

	return 0;
}















