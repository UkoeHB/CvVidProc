// implementation of TokenGeneratorAlgo for generating fragmented opencv video frames

#ifndef CV_VID_FRAMES_GENERATOR_ALGO_765678987_H
#define CV_VID_FRAMES_GENERATOR_ALGO_765678987_H

//local headers
#include "token_generator_algo.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cassert>
#include <memory>
#include <vector>


/// generator algorithm type declaration
class CvVidFramesGeneratorAlgo;

template <>
struct TokenGeneratorPack<CvVidFramesGeneratorAlgo> final
{
	/// total number of elements per batch (num frames x num chunks per frame)
	const int batch_size{};
	/// number of frames per batch
	const int frames_in_batch{};
	/// number of chunks to break each frame into
	const int chunks_per_frame{};
	/// path to video to get frames from
	const std::string vid_path{};
	/// first frame to grab for analysis (0-indexed)
	const long long start_frame{};
	/// lowest frame index not to process
	const long long last_frame{};
	/// rectangle for cropping the frames
	const cv::Rect crop_rectangle{};
	/// if frames should be converted to grayscale before being tokenized
	const bool convert_to_grayscale{};
	/// if video is already grayscale (optimization)
	const bool vid_is_grayscale{};
	/// horizontal buffer (pixels) on edge of each chunk (overlap region)
	const int horizontal_buffer_pixels{};
	/// vertical buffer (pixels) on edge of each chunk (overlap region)
	const int vertical_buffer_pixels{};
};

/// derive from this class with implementation of 'result handling'
/// extracts frames from a cv::VideoCapture and breaks them into chunks for tokenized batched processing
/// assumes pixels are defined with unsigned chars
class CvVidFramesGeneratorAlgo final : public TokenGeneratorAlgo<CvVidFramesGeneratorAlgo, typename cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	CvVidFramesGeneratorAlgo() = delete;

	/// normal constructor
	CvVidFramesGeneratorAlgo(TokenGeneratorPack<CvVidFramesGeneratorAlgo> param_pack) :
		TokenGeneratorAlgo{param_pack}
	{
		// open video
		m_vid = cv::VideoCapture{param_pack.vid_path};

		// sanity checks
		assert(m_pack.batch_size == m_pack.frames_in_batch*m_pack.chunks_per_frame);
		assert(m_vid.isOpened());

		int frame_width{static_cast<int>(m_vid.get(cv::CAP_PROP_FRAME_WIDTH))};
		int frame_height{static_cast<int>(m_vid.get(cv::CAP_PROP_FRAME_HEIGHT))};

		assert(m_pack.horizontal_buffer_pixels >= 0);
		assert(m_pack.vertical_buffer_pixels >= 0);
		assert(frame_width > 0);
		assert(frame_height > 0);
		assert(m_pack.frames_in_batch > 0 && m_pack.chunks_per_frame > 0);

		// make sure the Mats obtained will be composed of unsigned chars
		// http://ninghang.blogspot.com/2012/11/list-of-mat-type-in-opencv.html
		auto format{m_vid.get(cv::CAP_PROP_FORMAT)};
		assert(format == CV_8UC1 ||
				format == CV_8UC2 ||
				format == CV_8UC3 || 
				format == CV_8UC4);

		// make sure crop rectangle actually fits in the frame
		assert(m_pack.crop_rectangle.x + m_pack.crop_rectangle.width <= frame_width &&
			m_pack.crop_rectangle.y + m_pack.crop_rectangle.height <= frame_height);

		// start video on 'start frame'
		assert(m_pack.start_frame >= 0);
		assert(m_pack.start_frame < static_cast<int>(m_vid.get(cv::CAP_PROP_FRAME_COUNT)));
		m_vid.set(cv::CAP_PROP_POS_FRAMES, m_pack.start_frame);

		// validate last frame
		assert(m_pack.last_frame > 0);
		assert(m_pack.last_frame - m_pack.start_frame > 0);
		//note: if last_frame > num_frames ignore it

		// try to interpet video frames as RGB format for consistency (only when not grayscale already)
		if (!m_pack.vid_is_grayscale)
			m_vid.set(cv::CAP_PROP_CONVERT_RGB, true);
	}

	/// copy constructor: disabled
	CvVidFramesGeneratorAlgo(const CvVidFramesGeneratorAlgo&) = delete;

	/// destructor
	virtual ~CvVidFramesGeneratorAlgo() = default;

//overloaded operators
	/// asignment operator: disabled
	CvVidFramesGeneratorAlgo& operator=(const CvVidFramesGeneratorAlgo&) = delete;
	CvVidFramesGeneratorAlgo& operator=(const CvVidFramesGeneratorAlgo&) const = delete;

//member functions
	/// get token set from generator (batch of frames each chunked into segments)
	virtual std::vector<std::unique_ptr<cv::Mat>> GetTokenSet() override
	{
		token_set_type return_token_set{};
		std::size_t chunks_collected{0};

		for (std::size_t batch_index{0}; batch_index < m_pack.batch_size/m_pack.chunks_per_frame; batch_index++)
		{
			// leave if reached the last frame
			if (m_frames_consumed >= m_pack.last_frame - m_pack.start_frame)
				break;

			// get next frame from video
			cv::Mat frame{};
			m_vid >> frame;

			// leave if reached the end of the video or frame is corrupted
			if (!frame.data || frame.empty())
				break;

			// we will get data, so prepare the return vector (first pass-through)
			if (!return_token_set.size())
				return_token_set.resize(m_pack.batch_size);

			// crop the frame to desired size
			frame = frame(m_pack.crop_rectangle);

			// convert to grayscale if desired
			cv::Mat modified_frame{};

			if (m_pack.vid_is_grayscale)
				// video should already be grayscale, so directly get one channel (original grayscale frames have 3 channels)
				cv::extractChannel(frame, modified_frame, 0);
			else if (m_pack.convert_to_grayscale)
				// this should always work since the vid's CAP_PROP_CONVERT_RGB property was set
				cv::cvtColor(frame, modified_frame, cv::COLOR_RGB2GRAY);
			else
				modified_frame = std::move(frame);

			// break frame into chunks
			token_set_type temp_chunk_set{};

			if (m_pack.chunks_per_frame == 1)
				temp_chunk_set.emplace_back(std::make_unique<cv::Mat>(modified_frame));
			else if (!cv_mat_to_chunks(modified_frame, temp_chunk_set, static_cast<int>(m_pack.batch_size), 1, m_pack.horizontal_buffer_pixels, m_pack.vertical_buffer_pixels))
				std::cerr << "Breaking frame (" << m_frames_consumed + 1 << ") into chunks failed unexpectedly!\n";

			// store the set of chunks
			for (std::size_t set_index{0}; set_index < temp_chunk_set.size(); set_index++)
			{
				return_token_set[chunks_collected] = std::move(temp_chunk_set[set_index]);
				chunks_collected++;
			}

			m_frames_consumed++;
		}

		// reset if failed to get any frames/frame chunks
		if (!return_token_set.size())
		{
			// point video back to start frame
			m_vid.set(cv::CAP_PROP_POS_FRAMES, m_pack.start_frame);
			m_frames_consumed = 0;
		}

		return return_token_set;
	}

private:
//member variables
	/// video for processing
	cv::VideoCapture m_vid{};
	/// frame counter
	long long m_frames_consumed{0};
};


#endif //header guard













