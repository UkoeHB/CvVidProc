// helpers for getting background of an opencv vid

#ifndef CV_VID_BG_HELPERS_0089787_H
#define CV_VID_BG_HELPERS_0089787_H

//local headers
#include "cv_vid_frames_generator.h"
#include "cv_vid_fragment_consumer.h"
#include "async_token_process.h"
#include "histogram_median_algo.h"
#include "triframe_median_algo.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
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
	// max number of output fragments to store at a time (memory conservation vs efficiency)
	const int result_storage_limit{-1};
};

/// encapsulates call to async tokenized video background analysis
template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgo(cv::VideoCapture &vid, const VidBgPack &vidbg_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs)
{
	cv::Rect frame_dimensions{vidbg_pack.crop_x,
		vidbg_pack.crop_y,
		vidbg_pack.crop_width ? vidbg_pack.crop_width : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
		vidbg_pack.crop_height ? vidbg_pack.crop_height : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT))};

	// add another worker thread to make sure cores are well-utilized (heuristic)
	int temp_batch_size = vidbg_pack.batch_size + 1;

	// create frame generator
	auto frame_gen{std::make_shared<CvVidFramesGenerator>(temp_batch_size,
		vid,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels,
		vidbg_pack.frame_limit,
		frame_dimensions,
		vidbg_pack.grayscale)};

	// create fragment consumer
	auto bg_frag_consumer{std::make_shared<CvVidFragmentConsumer>(temp_batch_size,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels,
		frame_dimensions.width,
		frame_dimensions.height)};

	// create process
	AsyncTokenProcess<MedianAlgo, CvVidFragmentConsumer::final_result_type> vid_bg_prod{temp_batch_size,
		true,
		vidbg_pack.token_storage_limit,
		vidbg_pack.result_storage_limit,
		frame_gen,
		bg_frag_consumer};

	// run process to get background image
	auto bg_img{vid_bg_prod.Run(std::move(processor_packs))};

	if (bg_img && !bg_img->empty())
		return std::move(bg_img->back());
	else
		return cv::Mat{};
}

/// encapsulates call to async tokenized video background analysis using empty processor packs
template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const VidBgPack &vidbg_pack)
{
	std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
	empty_packs.resize(vidbg_pack.batch_size, TokenProcessorPack<MedianAlgo>{});

	return VidBackgroundWithAlgo<MedianAlgo>(vid, vidbg_pack, empty_packs);
}

/// get a video background
cv::Mat GetVideoBackground(const VidBgPack &vidbg_pack);


#endif //header guard





















