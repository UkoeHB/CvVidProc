// implementation of CvVidFramesConsumer for getting the background of an opencv video produced by a given algorithm

#ifndef CV_VID_BACKGROUND_CONSUMER_4567876_H
#define CV_VID_BACKGROUND_CONSUMER_4567876_H

//local headers
#include "token_batch_consumer.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>


/// tied to CvVidFramesGenerator implementation
class CvVidBackgroundConsumer final : public TokenBatchConsumer<cv::Mat, cv::Mat>
{
//member types
public:
	using TokenT = cv::Mat;
	using ParentT = TokenBatchConsumer<TokenT, cv::Mat>;

//constructors
	/// default constructor: disabled
	CvVidBackgroundConsumer() = delete;

	/// normal constructor
	CvVidBackgroundConsumer(const int batch_size,
			cv::VideoCapture &vid,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels) : 
		ParentT{batch_size},
		m_horizontal_buffer_pixels{horizontal_buffer_pixels},
		m_vertical_buffer_pixels{vertical_buffer_pixels},
		m_frame_width{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH))},
		m_frame_height{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT))}
	{
		// sanity checks
		assert(m_horizontal_buffer_pixels >= 0);
		assert(m_vertical_buffer_pixels >= 0);
		assert(m_frame_width > 0);
		assert(m_frame_height > 0);

		m_results.resize(GetBatchSize());
	}

	/// copy constructor: disabled
	CvVidBackgroundConsumer(const CvVidBackgroundConsumer&) = delete;

//destructor: not needed

//overloaded operators
	/// asignment operator: disabled
	CvVidBackgroundConsumer& operator=(const CvVidBackgroundConsumer&) = delete;
	CvVidBackgroundConsumer& operator=(const CvVidBackgroundConsumer&) const = delete;

protected:
//member functions
	/// consume an intermediate result from one of the token processing units
	/// replaces previous result for a given batch index with new results, so that in the end there is only one 'result batch'
	virtual void ConsumeToken(std::unique_ptr<TokenT> intermediate_result, const std::size_t index_in_batch) override
	{
		assert(index_in_batch < GetBatchSize());

		m_results[index_in_batch] = std::move(*intermediate_result);
	}

	/// get final result
	/// assembles final image fragments into a complete image
	/// WARNING: tied to implementation of fragment generator
	virtual std::unique_ptr<cv::Mat> GetFinalResult() override
	{
		// combine result fragments
		cv::Mat final_background{};
		if (!cv_mat_from_chunks(final_background,
				m_results,
				1,
				static_cast<int>(GetBatchSize()),
				m_frame_width,
				m_frame_height,
				m_horizontal_buffer_pixels,
				m_vertical_buffer_pixels)
			)
			std::cerr << "Combining final results into background image failed unexpectedly!\n";

		return std::make_unique<cv::Mat>(std::move(final_background));
	}

private:
//member variables
	/// horizontal buffer (pixels) on edge of each chunk (overlap region)
	const int m_horizontal_buffer_pixels{};
	/// vertical buffer (pixels) on edge of each chunk (overlap region)
	const int m_vertical_buffer_pixels{};
	/// width of each resulting frame (pixels)
	const int m_frame_width{};
	/// height of each resulting frame (pixels)
	const int m_frame_height{};

	/// store results (image fragment background) until they are ready to be used
	std::vector<TokenT> m_results{};
};


#endif //header guard
















