// helpers for getting background of an opencv vid

#ifndef CV_VID_BG_HELPERS_H
#define CV_VID_BG_HELPERS_H

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
#include <vector>


/// background algorithms available
enum class BGAlgo
{
	HISTOGRAM,
	TRIFRAME,
	UNKNOWN
};

/// get background algo from cv::String
BGAlgo GetBGAlgo(const cv::String &algo)
{
	if (algo == "hist")
		return BGAlgo::HISTOGRAM;
	else if (algo == "tri")
		return BGAlgo::TRIFRAME;
	else
	{
		std::cerr << "Unknown background algorithm detected: " << algo << '\n';
		return BGAlgo::UNKNOWN;
	}
}

/// settings necessary to get a vid background
struct VidBgPack
{
	// algorithm to use for getting vid bg
	const BGAlgo bg_algo{};
	// number of fragments each frame should be broken into (i.e. number of threads to use)
	const int batch_size{};

	// total number of frames in the video
	const long long total_frames{0};
	// max number of frames to analyze for getting the vig bg (<= 0 means use all frames in video)
	const long long frame_limit{-1};
	// whether to convert frames to grayscale before analyzing them
	const bool grayscale{false};

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
std::unique_ptr<cv::Mat> VidBackgroundWithAlgo(cv::VideoCapture &vid, const VidBgPack &vidbg_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs)
{
	// create frame generator
	auto frame_gen{std::make_shared<CvVidFramesGenerator>(vidbg_pack.batch_size,
		vid,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels,
		vidbg_pack.frame_limit,
		vidbg_pack.grayscale)};

	// create fragment consumer
	auto bg_frag_consumer{std::make_shared<CvVidFragmentConsumer>(vidbg_pack.batch_size,
		vid,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels)};

	// create process
	AsyncTokenProcess<MedianAlgo, CvVidFragmentConsumer::FinalResultT> vid_bg_prod{vidbg_pack.batch_size,
		vidbg_pack.token_storage_limit,
		vidbg_pack.result_storage_limit,
		frame_gen,
		bg_frag_consumer};

	// run process to get background image
	auto bg_img{vid_bg_prod.Run(std::move(processor_packs))};

	if (bg_img && !bg_img->empty())
		return std::make_unique<cv::Mat>(std::move(bg_img->back()));
	else
		return nullptr;
}

/// encapsulates call to async tokenized video background analysis using empty processor packs
template <typename MedianAlgo>
std::unique_ptr<cv::Mat> VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const VidBgPack &vidbg_pack)
{
	std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
	empty_packs.resize(vidbg_pack.batch_size, TokenProcessorPack<MedianAlgo>{});

	return VidBackgroundWithAlgo<MedianAlgo>(vid, vidbg_pack, empty_packs);
}

/// get a video background
std::unique_ptr<cv::Mat> GetVideoBackground(cv::VideoCapture &vid, const VidBgPack &vidbg_pack)
{
	// algo is user-specified
	switch (vidbg_pack.bg_algo)
	{
		case BGAlgo::HISTOGRAM :
		{
			// use cheapest histogram algorithm
			if (vidbg_pack.total_frames <= static_cast<long long>(static_cast<unsigned char>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo8>(vid, vidbg_pack);
			}
			else if (vidbg_pack.total_frames <= static_cast<long long>(static_cast<std::uint16_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo16>(vid, vidbg_pack);
			}
			else if (vidbg_pack.total_frames <= static_cast<long long>(static_cast<std::uint32_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo32>(vid, vidbg_pack);
			}
			else
			{
				std::cerr << "warning, video appears to have over 2^32 frames! (" << vidbg_pack.total_frames << ") is way too many!\n";
			}
		}

		case BGAlgo::TRIFRAME :
		{
			return VidBackgroundWithAlgoEmptyPacks<TriframeMedianAlgo>(vid, vidbg_pack);
		}

		default :
			return nullptr;
	};

	return nullptr;
}


#endif //header guard





















