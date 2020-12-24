// partial implementation of AsyncTokenProcess for consuming opencv video frames

#ifndef CV_VID_FRAMES_CONSUMER_765678987_H
#define CV_VID_FRAMES_CONSUMER_765678987_H

//local headers
#include "async_token_process.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>


/// derive from this class with implementation of 'result handling'
/// extracts frames from a cv::VideoCapture and breaks them into chunks for tokenized batched processing
template <typename FrameProcessorAlgoT, typename FinalResultT>
class CvVidFramesConsumer : public AsyncTokenProcess<FrameProcessorAlgoT, FinalResultT>
{
//member types
public:
	using TokenT = typename FrameProcessorAlgoT::token_type;

//constructors
	/// default constructor: disabled
	CvVidFramesConsumer() = delete;

	/// normal constructor
	CvVidFramesConsumer(cv::VideoCapture &vid,
			const int frame_limit,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels,
			const bool use_grayscale,
			const int worker_thread_limit,
			const int token_storage_limit,
			const int result_storage_limit) : 
		AsyncTokenProcess<FrameProcessorAlgoT, FinalResultT>{worker_thread_limit, token_storage_limit, result_storage_limit},
		m_vid{vid},
		m_frame_limit{frame_limit},
		m_convert_to_grayscale{use_grayscale},
		m_horizontal_buffer_pixels{horizontal_buffer_pixels},
		m_vertical_buffer_pixels{vertical_buffer_pixels}
	{
		static_assert(std::is_same<typename FrameProcessorAlgoT::token_type, cv::Mat>::value,
			"Frame processor implementation does not have token type cv::Mat!");

		// sanity checks
		assert(m_vid.isOpened());
		assert(m_horizontal_buffer_pixels >= 0);
		assert(m_vertical_buffer_pixels >= 0);

		// interpet video frames as RGB format for consistency
		vid.set(cv::CAP_PROP_CONVERT_RGB, true);
	}

	/// copy constructor: disabled
	CvVidFramesConsumer(const CvVidFramesConsumer&) = delete;

	/// destructor
	virtual ~CvVidFramesConsumer() = default;

//overloaded operators
	/// asignment operator: disabled
	CvVidFramesConsumer& operator=(const CvVidFramesConsumer&) = delete;
	CvVidFramesConsumer& operator=(const CvVidFramesConsumer&) const = delete;

protected:
//member functions
	/// get batch size (number of tokens in each batch)
	virtual int GetBatchSize() override = 0;

	/// get token set from generator (set of frame segments)
	virtual bool GetTokenSet(std::vector<std::unique_ptr<TokenT>> &return_token_set) override
	{
		cv::Mat frame{};

		// leave if reached frame limit
		if (m_frame_limit > 0 && m_frame_limit <= m_frames_consumed)
			return false;

		// get next frame from video
		m_vid >> frame;

		// leave if reached the end of the video or frame is corrupted
		if (!frame.data || frame.empty())
			return false;

		// get frame dimensions from first frame
		if (m_frames_consumed == 0)
		{
			// make sure the Mat produced is composed of unsigned chars
			// http://ninghang.blogspot.com/2012/11/list-of-mat-type-in-opencv.html
			assert(frame.type() == CV_8UC1 ||
					frame.type() == CV_8UC2 ||
					frame.type() == CV_8UC3 || 
					frame.type() == CV_8UC4);

			m_frame_width = frame.cols;
			m_frame_height = frame.rows;
		}

		// convert to grayscale if desired
		// this should always work since the vid's CAP_PROP_CONVERT_RGB property was set
		if (m_convert_to_grayscale)
		{
			cv::cvtColor(frame, frame, cv::COLOR_RGB2GRAY);
		}

		// break frame into chunks
		return_token_set.clear();		//expectation of cv_mat_to_chunks()

		if (!cv_mat_to_chunks(frame, return_token_set, 1, GetBatchSize(), m_horizontal_buffer_pixels, m_vertical_buffer_pixels))
			std::cerr << "Breaking frame (" << m_frames_consumed + 1 << ") into chunks failed unexpectedly!\n";

		m_frames_consumed++;

		return true;
	}

	/// reset in case the token process is restarted
	virtual void Reset() override
	{
		// reinitialize video; automatically calls '.release()'
		vid.open();

		m_frames_consumed = 0;
	}

protected:
//member variables
	/// horizontal buffer (pixels) on edge of each chunk (overlap region)
	const int m_horizontal_buffer_pixels{};
	/// vertical buffer (pixels) on edge of each chunk (overlap region)
	const int m_vertical_buffer_pixels{};
	/// width of each frame (pixels)
	int m_frame_width{};
	/// height of each frame (pixels)
	int m_frame_height{};

private:
	/// opencv video to get frames from
	cv::VideoCapture m_vid{};
	/// max number of frames to process (negative means go until end of vid)
	const int m_frame_limit{};
	/// if frames should be converted to grayscale before being tokenized
	const bool m_convert_to_grayscale{false};

	/// frame counter
	int m_frames_consumed{0};
};


#endif //header guard













