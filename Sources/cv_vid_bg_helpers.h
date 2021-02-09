// helpers for getting background of an opencv vid

#ifndef CV_VID_BG_HELPERS_0089787_H
#define CV_VID_BG_HELPERS_0089787_H

//local headers
#include "token_processor_algo_base.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <string>
#include <vector>


/// background algorithms available
enum class BGAlgo
{
	HISTOGRAM,
	TRIFRAME,
	UNKNOWN
};

/// get background algo from string
BGAlgo GetBGAlgo(const std::string &algo);

/// settings necessary to get a vid background
struct VidBgPack
{
	// path to video
	const std::string vid_path{};
	// algorithm to use for getting vid bg
	const std::string bg_algo{};
	// number of fragments each frame should be broken into (i.e. number of threads to use)
	const int batch_size{};

	// max number of frames to analyze for getting the vig bg (<= 0 means use all frames in video)
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

	// number of horizontal buffer pixels to add to analyzed fragments
	const int horizontal_buffer_pixels{0};
	// number of vertical buffer pixels to add to analyzed fragments
	const int vertical_buffer_pixels{0};

	// max number of input fragments to store at a time (memory conservation vs efficiency)
	const int token_storage_limit{-1};

	// whether to collect and print timing reports
	const bool print_timing_report{false};
};

/// encapsulates call to async tokenized video background analysis
template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgo(cv::VideoCapture &vid, const VidBgPack &vidbg_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs);

/// encapsulates call to async tokenized video background analysis using empty processor packs
template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const VidBgPack &vidbg_pack);

/// get a video background
cv::Mat GetVideoBackground(const VidBgPack &vidbg_pack);


#endif //header guard





















