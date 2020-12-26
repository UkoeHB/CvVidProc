// implementation of TokenBatchGenerator for generating fragmented opencv video frames

#ifndef CV_VID_FRAMES_GENERATOR_765678987_H
#define CV_VID_FRAMES_GENERATOR_765678987_H

//local headers
#include "token_batch_generator.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cassert>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>


/// derive from this class with implementation of 'result handling'
/// extracts frames from a cv::VideoCapture and breaks them into chunks for tokenized batched processing
/// assumes pixels are defined with unsigned chars
class CvVidFramesGenerator final : public TokenBatchGenerator<typename cv::Mat>
{
//member types
public:
	using TokenT = typename cv::Mat;
	using ParentT = TokenBatchGenerator<TokenT>;

//constructors
	/// default constructor: disabled
	CvVidFramesGenerator() = delete;

	/// normal constructor
	CvVidFramesGenerator(cv::VideoCapture &vid,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels,
			const long long frame_limit,
			const bool use_grayscale,
			const int batch_size) : 
		ParentT{batch_size},
		m_vid{vid},
		m_frame_limit{frame_limit},
		m_convert_to_grayscale{use_grayscale},
		m_horizontal_buffer_pixels{horizontal_buffer_pixels},
		m_vertical_buffer_pixels{vertical_buffer_pixels},
		m_frame_width{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH))},
		m_frame_height{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT))}
	{
		// sanity checks
		assert(m_vid.isOpened());
		assert(m_horizontal_buffer_pixels >= 0);
		assert(m_vertical_buffer_pixels >= 0);
		assert(m_frame_width > 0);
		assert(m_frame_height > 0);
		// make sure the Mats obtained will be composed of unsigned chars
		// http://ninghang.blogspot.com/2012/11/list-of-mat-type-in-opencv.html
		auto format{m_vid.get(cv::CAP_PROP_FORMAT)};
		assert(format == CV_8UC1 ||
				format == CV_8UC2 ||
				format == CV_8UC3 || 
				format == CV_8UC4);

		// interpet video frames as RGB format for consistency
		vid.set(cv::CAP_PROP_CONVERT_RGB, true);
	}

	/// copy constructor: disabled
	CvVidFramesGenerator(const CvVidFramesGenerator&) = delete;

	/// destructor
	virtual ~CvVidFramesGenerator() = default;

//overloaded operators
	/// asignment operator: disabled
	CvVidFramesGenerator& operator=(const CvVidFramesGenerator&) = delete;
	CvVidFramesGenerator& operator=(const CvVidFramesGenerator&) const = delete;

//member functions
	/// get token set from generator (set of frame segments)
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSet() override
	{
		cv::Mat frame{};

		// leave if reached frame limit
		if (m_frame_limit > 0 && m_frame_limit <= m_frames_consumed)
			return std::vector<std::unique_ptr<TokenT>>{};

		// get next frame from video
		m_vid >> frame;

		// leave if reached the end of the video or frame is corrupted
		if (!frame.data || frame.empty())
			return std::vector<std::unique_ptr<TokenT>>{};

		// convert to grayscale if desired
		// this should always work since the vid's CAP_PROP_CONVERT_RGB property was set
		if (m_convert_to_grayscale)
		{
			cv::cvtColor(frame, frame, cv::COLOR_RGB2GRAY);
		}

		// break frame into chunks
		std::vector<std::unique_ptr<TokenT>> return_token_set{};

		if (!cv_mat_to_chunks(frame, return_token_set, 1, static_cast<int>(GetBatchSize()), m_horizontal_buffer_pixels, m_vertical_buffer_pixels))
			std::cerr << "Breaking frame (" << m_frames_consumed + 1 << ") into chunks failed unexpectedly!\n";

		m_frames_consumed++;

		return return_token_set;
	}

	/// reset in case the token process is restarted
	virtual void ResetGenerator() override
	{
		// point video back to first frame
		m_vid.set(cv::CAP_PROP_FRAME_COUNT, 0);

		m_frames_consumed = 0;
	}

private:
//member variables
	/// horizontal buffer (pixels) on edge of each chunk (overlap region)
	const int m_horizontal_buffer_pixels{};
	/// vertical buffer (pixels) on edge of each chunk (overlap region)
	const int m_vertical_buffer_pixels{};
	/// width of each frame (pixels)
	const int m_frame_width{};
	/// height of each frame (pixels)
	const int m_frame_height{};

	/// opencv video to get frames from
	cv::VideoCapture m_vid{};
	/// max number of frames to process (negative means go until end of vid)
	const long long m_frame_limit{};
	/// if frames should be converted to grayscale before being tokenized
	const bool m_convert_to_grayscale{false};

	/// frame counter
	long long m_frames_consumed{0};
};


#endif //header guard













