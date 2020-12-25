// implementation of CvVidFramesConsumer for getting the background of an opencv video using a given algorithm

#ifndef CV_VID_BACKGROUND_4567876_H
#define CV_VID_BACKGROUND_4567876_H

//local headers
#include "cv_vid_frames_consumer.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>


template <typename FrameProcessorAlgoT>
class CvVidBackground final : public CvVidFramesConsumer<FrameProcessorAlgoT, cv::Mat>
{
//member types
public:
	using TokenT = typename FrameProcessorAlgoT::token_type;
	using ResultT = typename FrameProcessorAlgoT::result_type;
	using PPackSetT = std::vector<TokenProcessorPack<FrameProcessorAlgoT>>;
	using ParentT = CvVidFramesConsumer<FrameProcessorAlgoT, cv::Mat>;

//constructors
	/// default constructor: disabled
	CvVidBackground() = delete;

	/// normal constructor
	CvVidBackground(PPackSetT &processor_packs,
			cv::VideoCapture &vid,
			const long long frame_limit,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels,
			const bool use_grayscale,		
			const int worker_thread_limit,
			const int token_storage_limit,
			const int result_storage_limit) : 
		m_processor_packs{processor_packs},
		ParentT{vid, frame_limit, horizontal_buffer_pixels, vertical_buffer_pixels, use_grayscale, worker_thread_limit, token_storage_limit, result_storage_limit},
		m_batch_size{worker_thread_limit}
	{
		assert(m_processor_packs.size() == static_cast<std::size_t>(worker_thread_limit));

		m_results.resize(m_batch_size);
	}

	/// copy constructor: disabled
	CvVidBackground(const CvVidBackground&) = delete;

//destructor: not needed

//overloaded operators
	/// asignment operator: disabled
	CvVidBackground& operator=(const CvVidBackground&) = delete;
	CvVidBackground& operator=(const CvVidBackground&) const = delete;

protected:
//member functions
	/// get batch size (number of tokens in each batch)
	virtual int GetBatchSize() override { return m_batch_size; }

	/// get processing packs for processing units (num packs = intended batch size)
	virtual PPackSetT GetProcessingPacks() override { return m_processor_packs; }

	/// consume an intermediate result from one of the token processing units
	/// replaces previous result for a given batch index with new results, so that in the end there is only one 'result batch'
	virtual void ConsumeResult(std::unique_ptr<ResultT> intermediate_result, const std::size_t index_in_batch) override
	{
		assert(index_in_batch < static_cast<std::size_t>(m_batch_size));

		m_results[index_in_batch] = std::move(*intermediate_result);
	}

	/// get final result
	/// assembles final image fragments into a complete image
	virtual std::unique_ptr<cv::Mat> GetFinalResult() override
	{
		// combine result fragments
		cv::Mat final_background{};
		if (!cv_mat_from_chunks(final_background,
				m_results,
				1,
				m_batch_size,
				ParentT::m_frame_width,
				ParentT::m_frame_height,
				ParentT::m_horizontal_buffer_pixels,
				ParentT::m_vertical_buffer_pixels)
			)
			std::cerr << "Combining final results into background image failed unexpectedly!\n";

		return std::make_unique<cv::Mat>(std::move(final_background));
	}

private:
//member variables
	/// batch size (number of frame segments per batch)
	const int m_batch_size{};
	/// variable packs to initialize all workers running the FrameProcessorAlgoT
	PPackSetT m_processor_packs{};

	/// store results (image fragment background) until they are ready to be used
	std::vector<ResultT> m_results{};
};


#endif //header guard


//ignores all except very last result from all token processing units (unless specialized)














