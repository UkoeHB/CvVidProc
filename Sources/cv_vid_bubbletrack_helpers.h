// helper functions for executing bubble tracking

#ifndef CV_VID_BUBBLETRACK_HELPERS_4738498_H
#define CV_VID_BUBBLETRACK_HELPERS_4738498_H

//local headers
#include "assign_bubbles_algo.h"
#include "highlight_bubbles_algo.h"

//third party headers
#include <pybind11/pybind11.h>
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <memory>
#include <string>
#include <vector>


/// settings necessary to track bubbles in a video
struct __attribute__ ((visibility("hidden"))) VidBubbleTrackPack
{
	// path to video
	const std::string vid_path{};

	// pack of variables for HighlightBubblesAlgo
	TokenProcessorPack<HighlightBubblesAlgo> highlightbubbles_pack;
	// pack of variables for AssignBubblesAlgo
	TokenProcessorPack<AssignBubblesAlgo> assignbubbles_pack;

	// max number of threads allowed
	const int max_threads{-1};

	// max number of frames to analyze (<= 0 means use all frames in video)
	const long long frame_limit{-1};
	// whether to convert frames to grayscale before analyzing them
	const bool grayscale{false};
	// whether video is already supposedly grayscale
	const bool vid_is_grayscale{false};

	// x-position of frame-crop region
	const int crop_x{0};
	// y-position of frame-crop region
	const int crop_y{0};
	// width of frame-crop region
	const int crop_width{0};
	// height of frame-crop region
	const int crop_height{0};

	// max number of input fragments to store at a time (memory conservation vs efficiency)
	const int token_storage_limit{-1};

	// whether to collect and print timing reports
	const bool print_timing_report{false};
};

/// encapsulates call to async tokenized bubble tracking analysis
std::unique_ptr<py::dict> TrackBubblesProcess(cv::VideoCapture &vid,
	const VidBubbleTrackPack &trackbubble_pack,
	std::vector<TokenProcessorPack<HighlightBubblesAlgo>> &highlightbubbles_packs,
	std::vector<TokenProcessorPack<AssignBubblesAlgo>> &assignbubbles_packs);

/// track bubbles in a video and return record of bubbles tracked
/// WARNING: can only be called when the python GIL is held
py::dict TrackBubbles(const VidBubbleTrackPack &trackbubble_pack);


#endif //header guard














