// helpers for getting background of an opencv vid

#ifndef MAIN_9889789_H
#define MAIN_9889789_H

//local headers

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers


struct CommandLinePack
{
	cv::String vid_path{};
	int worker_threads{};
	bool grayscale{};
	long long bg_frame_lim{};
	cv::String bg_algo{};
};

/// get number of worker threads to use (subtract one for the main thread)
/// note: min threads is 2 (1 for main thread, 1 for worker)
int WorkerThreadsFromMax(int max_threads);

/// interpret the command line arguments
CommandLinePack HandleCLArgs(cv::CommandLineParser &cl_args);

/// convert a CommandLinePack to VidBgPack
VidBgPack vidbgpack_from_clpack(const CommandLinePack &cl_pack, const int threads, const long long total_frames);



#endif  //header guard






















