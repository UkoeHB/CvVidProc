// helpers for getting background of an opencv vid

#ifndef MAIN_9889789_H
#define MAIN_9889789_H

//local headers
#include "cv_vid_bg_helpers.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers


struct CommandLinePack
{
	cv::String vid_path{};
	int max_threads{};
	bool grayscale{};
	bool vid_is_grayscale{};
	long long bg_frame_lim{};
	cv::String bg_algo{};
	bool print_timing_report{};
};

/// get number of additional threads available above min_threads, plus extra_threads, but not more than max_threads
int GetAdditionalThreads(int min_threads, int extra_threads = 0, int max_threads = -1);

/// interpret the command line arguments
CommandLinePack HandleCLArgs(cv::CommandLineParser &cl_args);

/// convert a CommandLinePack to VidBgPack
VidBgPack vidbgpack_from_clpack(const CommandLinePack &cl_pack, const int threads, const long long total_frames);



#endif  //header guard




















