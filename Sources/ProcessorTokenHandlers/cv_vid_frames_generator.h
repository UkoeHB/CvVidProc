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
#include <chrono>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>


/// derive from this class with implementation of 'result handling'
/// extracts frames from a cv::VideoCapture and breaks them into chunks for tokenized batched processing
/// assumes pixels are defined with unsigned chars
class CvVidFramesGenerator final : public TokenBatchGenerator<typename cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	CvVidFramesGenerator() = delete;

	/// normal constructor
	CvVidFramesGenerator(const int frames_in_batch,
			const int chunks_per_frame,
			const bool collect_timings,
			cv::VideoCapture &vid,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels,
			const long long frame_limit,
			const cv::Rect crop_rectangle,
			const bool use_grayscale,
			const bool vid_is_grayscale) : 
		TokenBatchGenerator{frames_in_batch*chunks_per_frame, collect_timings},
		m_vid{vid},
		m_frame_limit{frame_limit},
		m_chunks_per_frame{chunks_per_frame},
		m_crop_rectangle{crop_rectangle},
		m_convert_to_grayscale{use_grayscale},
		m_vid_is_grayscale{vid_is_grayscale},
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
		assert(frames_in_batch > 0 && chunks_per_frame > 0);

		// make sure the Mats obtained will be composed of unsigned chars
		// http://ninghang.blogspot.com/2012/11/list-of-mat-type-in-opencv.html
		auto format{m_vid.get(cv::CAP_PROP_FORMAT)};
		assert(format == CV_8UC1 ||
				format == CV_8UC2 ||
				format == CV_8UC3 || 
				format == CV_8UC4);

		// make sure crop rectangle actually fits in the frame
		assert(m_crop_rectangle.x + m_crop_rectangle.width <= m_frame_width &&
			m_crop_rectangle.y + m_crop_rectangle.height <= m_frame_height);

		// try to interpet video frames as RGB format for consistency (only when not grayscale already)
		if (!m_vid_is_grayscale)
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
protected:
	/// get token set from generator (batch of frames each chunked into segments)
	virtual std::vector<std::unique_ptr<token_type>> GetTokenSetImpl() override
	{
		using return_t = std::vector<std::unique_ptr<token_type>>;
		return_t return_token_set{};
		std::size_t chunks_collected{0};

		for (std::size_t batch_index{0}; batch_index < GetBatchSize()/m_chunks_per_frame; batch_index++)
		{
			// leave if reached frame limit
			if (m_frame_limit > 0 && m_frames_consumed >= m_frame_limit)
				break;

			// get next frame from video
			cv::Mat frame{};
			m_vid >> frame;

			// leave if reached the end of the video or frame is corrupted
			if (!frame.data || frame.empty())
				break;

			// we will get data, so prepare the return vector (first pass-through)
			if (!return_token_set.size())
				return_token_set.resize(GetBatchSize());

			// crop the frame to desired size
			frame = frame(m_crop_rectangle);

			// convert to grayscale if desired
			cv::Mat modified_frame{};

			if (m_vid_is_grayscale)
				// video should already be grayscale, so directly get one channel (original grayscale frames have 3 channels)
				cv::extractChannel(frame, modified_frame, 0);
			else if (m_convert_to_grayscale)
				// this should always work since the vid's CAP_PROP_CONVERT_RGB property was set
				cv::cvtColor(frame, modified_frame, cv::COLOR_RGB2GRAY);
			else
				modified_frame = std::move(frame);

			// break frame into chunks
			return_t temp_chunk_set{};

			if (m_chunks_per_frame == 1)
				temp_chunk_set.emplace_back(std::make_unique<cv::Mat>(modified_frame));
			else if (!cv_mat_to_chunks(modified_frame, temp_chunk_set, 1, static_cast<int>(GetBatchSize()), m_horizontal_buffer_pixels, m_vertical_buffer_pixels))
				std::cerr << "Breaking frame (" << m_frames_consumed + 1 << ") into chunks failed unexpectedly!\n";

			// store the set of chunks
			for (std::size_t set_index{0}; set_index < temp_chunk_set.size(); set_index++)
			{
				return_token_set[chunks_collected] = std::move(temp_chunk_set[set_index]);
				chunks_collected++;
			}

			m_frames_consumed++;
		}

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
	/// rectangle for cropping the frames
	const cv::Rect m_crop_rectangle{};
	/// if frames should be converted to grayscale before being tokenized
	const bool m_convert_to_grayscale{};
	/// if video is already grayscale (optimization)
	const bool m_vid_is_grayscale{};
	/// number of chunks to break each frame into
	const int m_chunks_per_frame{};

	/// frame counter
	long long m_frames_consumed{0};
};


#endif //header guard













