// helpers for getting background of an opencv vid

//local headers
#include "cv_vid_bg_helpers.h"
#include "histogram_median_algo.h"
#include "triframe_median_algo.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>


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
				  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT) <<
				  "; FPS: " << vid.get(cv::CAP_PROP_FPS) << '\n';

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




















