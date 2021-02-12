// helpers for getting background of an opencv vid

//local headers
#include "async_token_process.h"
#include "cv_vid_bg_helpers.h"
#include "cv_vid_frames_generator.h"
#include "cv_vid_fragment_consumer.h"
#include "histogram_median_algo.h"
#include "main.h"
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


/// get background algo from string
BGAlgo GetBGAlgo(const std::string &algo)
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

template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgo(cv::VideoCapture &vid, const VidBgPack &vidbg_pack, std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs)
{
	// number of fragments to create during background analysis
	int batch_size{static_cast<int>(processor_packs.size())};
	assert(batch_size);

	cv::Rect frame_dimensions{vidbg_pack.crop_x,
		vidbg_pack.crop_y,
		vidbg_pack.crop_width ? vidbg_pack.crop_width : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
		vidbg_pack.crop_height ? vidbg_pack.crop_height : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT))};

	// create frame generator
	auto frame_gen{std::make_shared<CvVidFramesGenerator>(1,
		batch_size,
		vidbg_pack.print_timing_report,
		vid,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels,
		vidbg_pack.frame_limit,
		frame_dimensions,
		vidbg_pack.grayscale,
		vidbg_pack.vid_is_grayscale)};

	// create fragment consumer
	auto bg_frag_consumer{std::make_shared<CvVidFragmentConsumer>(batch_size,
		vidbg_pack.print_timing_report,
		vidbg_pack.horizontal_buffer_pixels,
		vidbg_pack.vertical_buffer_pixels,
		frame_dimensions.width,
		frame_dimensions.height)};

	// create process
	AsyncTokenProcess<MedianAlgo, CvVidFragmentConsumer::final_result_type> vid_bg_prod{batch_size,
		true,
		vidbg_pack.print_timing_report,
		vidbg_pack.token_storage_limit,
		vidbg_pack.token_storage_limit,
		frame_gen,
		bg_frag_consumer};

	// run process to get background image
	auto bg_img{vid_bg_prod.Run(std::move(processor_packs))};

	// print out timing info
	if (vidbg_pack.print_timing_report)
		std::cout << vid_bg_prod.GetTimingInfoAndResetTimer();

	if (bg_img && !bg_img->empty())
		return std::move(bg_img->back());
	else
		return cv::Mat{};
}

template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const VidBgPack &vidbg_pack)
{
	int batch_size{GetAdditionalThreads(1, 1, vidbg_pack.max_threads)};

	std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
	empty_packs.resize(batch_size, TokenProcessorPack<MedianAlgo>{});

	return VidBackgroundWithAlgo<MedianAlgo>(vid, vidbg_pack, empty_packs);
}

/// get a video background
cv::Mat GetVideoBackground(const VidBgPack &vidbg_pack)
{
	// open video file
	cv::VideoCapture vid{vidbg_pack.vid_path};

	if (!vid.isOpened())
	{
		std::cerr << "Video file not detected: " << vidbg_pack.vid_path << '\n';

		return cv::Mat{};
	}

	// print info about the video
	long long total_frames{static_cast<long long>(vid.get(cv::CAP_PROP_FRAME_COUNT))};

	std::cout << "Frames: " << total_frames <<
				  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT);
	if (vidbg_pack.crop_width || vidbg_pack.crop_height)
	{
		int width = vidbg_pack.crop_width ? vidbg_pack.crop_width : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH));
		int height = vidbg_pack.crop_height ? vidbg_pack.crop_height : static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT));
		std::cout << "(" << width << 'x' << height << " cropped)";
	}
	std::cout << "; FPS: " << vid.get(cv::CAP_PROP_FPS) << '\n';

	// figure out how many frames will be analyzed
	long long frames_to_analyze{vidbg_pack.frame_limit};

	if (frames_to_analyze <= 0 || frames_to_analyze > total_frames)
		frames_to_analyze = total_frames;

	// algo is user-specified
	switch (GetBGAlgo(vidbg_pack.bg_algo))
	{
		case BGAlgo::HISTOGRAM :
		{
			// use cheapest histogram algorithm
			if (frames_to_analyze <= static_cast<long long>(static_cast<unsigned char>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo8>(vid, vidbg_pack);
			}
			else if (frames_to_analyze <= static_cast<long long>(static_cast<std::uint16_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo16>(vid, vidbg_pack);
			}
			else if (frames_to_analyze <= static_cast<long long>(static_cast<std::uint32_t>(-1)))
			{
				return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo32>(vid, vidbg_pack);
			}
			else
			{
				std::cerr << "warning, video appears to have over 2^32 frames! (" << total_frames << ") is way too many!\n";
			}
		}

		case BGAlgo::TRIFRAME :
		{
			return VidBackgroundWithAlgoEmptyPacks<TriframeMedianAlgo>(vid, vidbg_pack);
		}

		default :
		{
			std::cerr << "tried to get vid background with unknown algorithm: " << vidbg_pack.bg_algo << '\n';

			return cv::Mat{};
		}
	};

	return cv::Mat{};
}




















