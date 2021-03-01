// helpers for executing bubble tracking

//local headers
#include "assign_bubbles_algo.h"
#include "async_token_batch_generator.h"
#include "async_token_process.h"
#include "cv_vid_bubbletrack_helpers.h"
#include "cv_vid_frames_generator_algo.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "mat_set_intermediary.h"
#include "py_dict_consumer.h"

//third party headers
#include <pybind11/pybind11.h>
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <future>
#include <iostream>
#include <memory>
#include <vector>


/// encapsulates call to async tokenized bubble tracking analysis
std::unique_ptr<py::dict> TrackBubblesProcess(cv::VideoCapture &vid,
	const VidBubbleTrackPack &trackbubble_pack,
	std::vector<TokenProcessorPack<HighlightBubblesAlgo>> &highlightbubbles_packs,
	std::vector<TokenProcessorPack<AssignBubblesAlgo>> &assignbubbles_packs)
{
	// we must have the gil so resource cleanup does not cause segfaults
	//TODO: figure out how to release gil here
	//  (segfault occurs at return of AsyncTokenProcess<AssignBubblesAlgo, PyDictConsumer::final_result_type>::Run())
	py::gil_scoped_acquire gil;

	// expect only one assignbbubbles_pack
	assert(assignbubbles_packs.size() == 1);

	// set batch size
	int batch_size{static_cast<int>(highlightbubbles_packs.size())};
	assert(batch_size);

	// create crop-window for processing frames
	cv::Rect frame_dimensions{GetCroppedFrameDims(trackbubble_pack.crop_x, trackbubble_pack.crop_y, trackbubble_pack.crop_width, trackbubble_pack.crop_height,
		static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
		static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)))};

	/// create frame generator

	// cap range of frames to analyze at the frame limit
	long long num_frames{};
	if (vid.isOpened())
	{
		num_frames = static_cast<long long>(vid.get(cv::CAP_PROP_FRAME_COUNT));
		
		// if frame limit is <= 0 use default (all frames in vid)
		if (trackbubble_pack.frame_limit > 0)
		{
			if (num_frames > trackbubble_pack.frame_limit)
				num_frames = trackbubble_pack.frame_limit;
		}
	}
	else
		return nullptr;

	// frame generator packs
	std::vector<TokenGeneratorPack<CvVidFramesGeneratorAlgo>> generator_packs{};
	generator_packs.emplace_back(TokenGeneratorPack<CvVidFramesGeneratorAlgo>{
		batch_size,
		batch_size,
		1,  // frames are not chunked
		trackbubble_pack.vid_path,
		0,
		num_frames,
		frame_dimensions,
		trackbubble_pack.grayscale,
		trackbubble_pack.vid_is_grayscale,
		0,
		0
	});

	// frame generator
	auto frame_gen{std::make_shared<AsyncTokenBatchGenerator<CvVidFramesGeneratorAlgo>>(
		batch_size,
		trackbubble_pack.print_timing_report,
		trackbubble_pack.token_storage_limit
	)};

	frame_gen->StartGenerator(std::move(generator_packs));

	// create mat shuttle that passes frames with highlighted bubbles to assign bubbles algo
	auto mat_shuttle{std::make_shared<MatSetIntermediary>(batch_size,
		trackbubble_pack.print_timing_report,
		trackbubble_pack.token_storage_limit)};

	// create consumer that collects final bubbles archive
	auto dict_collector{std::make_shared<PyDictConsumer>(1,
		trackbubble_pack.print_timing_report)};

	// create process for highlighting bubbles
	using highlight_bubbles_proc_t = AsyncTokenProcess<HighlightBubblesAlgo, MatSetIntermediary::final_result_type>;
	highlight_bubbles_proc_t highlight_bubbles_proc{
		batch_size,
		true,
		trackbubble_pack.print_timing_report,
		trackbubble_pack.token_storage_limit,
		trackbubble_pack.token_storage_limit,
		frame_gen,
		mat_shuttle};

	// create process for identifying and tracking bubbles
	AsyncTokenProcess<AssignBubblesAlgo, PyDictConsumer::final_result_type> assign_bubbles_proc{
		1,
		true,
		trackbubble_pack.print_timing_report,
		trackbubble_pack.token_storage_limit,
		trackbubble_pack.token_storage_limit,
		mat_shuttle,	// note:: token consumer of previous process is generator for this process
		dict_collector};

	// start the highlighting bubbles process in a new thread
	auto hbproc_handle = std::async(&highlight_bubbles_proc_t::Run, &highlight_bubbles_proc, std::move(highlightbubbles_packs));

	// run the assign bubbles process in this thread (should run synchronously)
	auto bubble_archive{assign_bubbles_proc.Run(std::move(assignbubbles_packs))};

	// close out the highlight bubbles thread (it should be waiting at this point)
	// note: we don't care about the return value
	hbproc_handle.get();

	// print out timing info
	if (trackbubble_pack.print_timing_report)
	{
		std::cout << "Highlight bubbles timing report:\n";
		std::cout << highlight_bubbles_proc.GetTimingInfoAndResetTimer();

		std::cout << "Assign bubbles timing report:\n";
		std::cout << assign_bubbles_proc.GetTimingInfoAndResetTimer();
	}

	// return the dictionary of all bubbles tracked
	if (bubble_archive && bubble_archive->size())
		return std::move(bubble_archive->front());
	else
		return nullptr;
}

/// WARNING: can only be called when the python GIL is held
py::dict TrackBubbles(const VidBubbleTrackPack &trackbubble_pack)
{
	// open video file
	cv::VideoCapture vid{trackbubble_pack.vid_path};

	if (!vid.isOpened())
	{
		std::cerr << "Video file not detected: " << trackbubble_pack.vid_path << '\n';

		return py::dict{};
	}

	// validate inputs
	// frame cropping should match background image passed in
	assert(trackbubble_pack.highlightbubbles_pack.background.data && !trackbubble_pack.highlightbubbles_pack.background.empty());
	assert(trackbubble_pack.crop_width - trackbubble_pack.crop_x >= 0);
	assert(trackbubble_pack.crop_height - trackbubble_pack.crop_y >= 0);
	assert(trackbubble_pack.crop_width ?
		trackbubble_pack.crop_width - trackbubble_pack.crop_x == trackbubble_pack.highlightbubbles_pack.background.cols :
		static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)) == trackbubble_pack.highlightbubbles_pack.background.cols);
	assert(trackbubble_pack.crop_height ?
		trackbubble_pack.crop_height - trackbubble_pack.crop_y == trackbubble_pack.highlightbubbles_pack.background.rows :
		static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)) == trackbubble_pack.highlightbubbles_pack.background.rows);

	// the structuring element should exist
	assert(trackbubble_pack.highlightbubbles_pack.struct_element.data && !trackbubble_pack.highlightbubbles_pack.struct_element.empty());

	/// create algo packs

	// get number of frames to highlight in parallel based on number of threads available
	// 3 -> min threads required; 0 -> add no extra threads (heuristic from testing)
	// + 1 -> roll one of the required threads into the additional threads obtained to get the batch size
	int batch_size{GetAdditionalThreads(3, 0, trackbubble_pack.max_threads) + 1};

	// highlight bubbles algo packs
	std::vector<TokenProcessorPack<HighlightBubblesAlgo>> highlightbubbles_packs{};
	highlightbubbles_packs.reserve(batch_size);
	for (int i{0}; i < batch_size; i++)
	{
		// all packs are copies of the input pack
		highlightbubbles_packs.emplace_back(trackbubble_pack.highlightbubbles_pack);

		// must manually clone the cv::Mats here because their copy constructor only creates a reference
		highlightbubbles_packs[i].background = trackbubble_pack.highlightbubbles_pack.background.clone();
		highlightbubbles_packs[i].struct_element = trackbubble_pack.highlightbubbles_pack.struct_element.clone();
	}

	// there is only one assignbubbles pack
	std::vector<TokenProcessorPack<AssignBubblesAlgo>> assignbubbles_packs{};
	assignbubbles_packs.emplace_back(trackbubble_pack.assignbubbles_pack);

	// call the process
	std::unique_ptr<py::dict> bubbles_archive{
		TrackBubblesProcess(vid, trackbubble_pack, highlightbubbles_packs, assignbubbles_packs)};

	// return the dictionary of tracked bubbles
	if (bubbles_archive)
		return std::move(*(bubbles_archive.release()));
	else
		return py::dict{};
}



















