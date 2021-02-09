// helpers for executing bubble tracking

//local headers
#include "assign_bubbles_algo.h"
#include "async_token_process.h"
#include "cv_vid_bubbletrack_helpers.h"
#include "cv_vid_frames_generator.h"
#include "highlight_bubbles_algo.h"
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
	// this function releases the GIL so called functions can acquire it
	//py::gil_scoped_release nogil;

	// expect only one assignbbubbles_pack
	assert(assignbubbles_packs.size() == 1);

	// create crop-window for processing frames
	cv::Rect frame_dimensions{trackbubble_pack.crop_x,
		trackbubble_pack.crop_y,
		trackbubble_pack.crop_width ? trackbubble_pack.crop_width : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
		trackbubble_pack.crop_height ? trackbubble_pack.crop_height : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT))};

	// create frame generator
	auto frame_gen{std::make_shared<CvVidFramesGenerator>(trackbubble_pack.batch_size,
		1,	// frames are not chunked
		trackbubble_pack.print_timing_report,
		vid,
		0,
		0,
		trackbubble_pack.frame_limit,
		frame_dimensions,
		trackbubble_pack.grayscale,
		trackbubble_pack.vid_is_grayscale)};

	// create mat shuttle that passes frames with highlighted bubbles to assign bubbles algo
	auto mat_shuttle{std::make_shared<MatSetIntermediary>(trackbubble_pack.batch_size,
		trackbubble_pack.token_storage_limit,
		trackbubble_pack.print_timing_report)};

	// create consumer that collects final bubbles archive
	auto dict_collector{std::make_shared<PyDictConsumer>(1,
		trackbubble_pack.print_timing_report)};

	// create process for highlighting bubbles
	using highlight_bubbles_proc_t = AsyncTokenProcess<HighlightBubblesAlgo, MatSetIntermediary::final_result_type>;
	highlight_bubbles_proc_t highlight_bubbles_proc{
		trackbubble_pack.batch_size,
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
	if (!hbproc_handle.get())
		std::cout << "Unexpectedly not all highlighted images were cleared from highlight_bubbles_proc!\n";

	// print out timing info
	if (trackbubble_pack.print_timing_report)
	{
		std::cout << "Highlight bubbles timing report:\n";
		std::cout << highlight_bubbles_proc.GetTimingInfoAndResetTimer();

		std::cout << "Assign bubbles timing report:\n";
		std::cout << assign_bubbles_proc.GetTimingInfoAndResetTimer();
	}
std::cerr << "7\n";
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

	// create algo packs

	// highlight bubbles algo packs
	std::vector<TokenProcessorPack<HighlightBubblesAlgo>> highlightbubbles_packs{};
	highlightbubbles_packs.reserve(trackbubble_pack.batch_size);
	for (int i{0}; i < trackbubble_pack.batch_size; i++)
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



















