// implementation of TokenBatchConsumer for obtaining image fragments of type cv::Mat and assembling them

#ifndef CV_VID_BG_FRAGMENT_CONSUMER_4567876_H
#define CV_VID_BG_FRAGMENT_CONSUMER_4567876_H

//local headers
#include "token_batch_consumer.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <vector>


/// tied to CvVidFramesGenerator implementation
/// assumes result fragments are passed out in layers (each full set of fragments is stitched together without further thought)
class CvVidFragmentConsumer final : public TokenBatchConsumer<cv::Mat, std::list<cv::Mat>>
{
public:
//constructors
	/// default constructor: disabled
	CvVidFragmentConsumer() = delete;

	/// normal constructor
	CvVidFragmentConsumer(const int batch_size,
			const bool collect_timings,
			const int horizontal_buffer_pixels,
			const int vertical_buffer_pixels,
			const int frame_width,
			const int frame_height) : 
		TokenBatchConsumer{batch_size, collect_timings},
		m_horizontal_buffer_pixels{horizontal_buffer_pixels},
		m_vertical_buffer_pixels{vertical_buffer_pixels},
		m_frame_width{frame_width},
		m_frame_height{frame_height}
	{
		// sanity checks
		assert(m_horizontal_buffer_pixels >= 0);
		assert(m_vertical_buffer_pixels >= 0);
		assert(m_frame_width > 0);
		assert(m_frame_height > 0);

		m_fragments.resize(GetBatchSize());
	}

	/// copy constructor: disabled
	CvVidFragmentConsumer(const CvVidFragmentConsumer&) = delete;

//destructor: not needed

//overloaded operators
	/// asignment operator: disabled
	CvVidFragmentConsumer& operator=(const CvVidFragmentConsumer&) = delete;
	CvVidFragmentConsumer& operator=(const CvVidFragmentConsumer&) const = delete;

protected:
//member functions
	/// consume an image fragment
	/// WARNING: tied to implementation of fragment generator
	virtual void ConsumeTokenImpl(std::unique_ptr<token_type> intermediate_result, const std::size_t index_in_batch) override
	{
		assert(index_in_batch < GetBatchSize());

		m_fragments[index_in_batch].emplace_back(std::move(*intermediate_result));

		// check if a full image can be assembled
		bool no_luck{false};

		for (const auto &frag_list : m_fragments)
		{
			if (frag_list.empty())
			{
				no_luck = true;

				break;
			}
		}

		// add result if possible
		if (!no_luck)
		{
			// pull out a full image
			std::vector<cv::Mat> img_frags{};
			img_frags.resize(GetBatchSize());

			for (std::size_t batch_index{0}; batch_index < GetBatchSize(); batch_index++)
			{
				img_frags[batch_index] = std::move(m_fragments[batch_index].front());
				m_fragments[batch_index].pop_front();
			}

			// combine result fragments
			cv::Mat result_img{};
			if (!cv_mat_from_chunks(result_img,
					img_frags,
					1,
					static_cast<int>(GetBatchSize()),
					m_frame_width,
					m_frame_height,
					m_horizontal_buffer_pixels,
					m_vertical_buffer_pixels)
				)
				std::cerr << "Combining img fragments into image failed unexpectedly!\n";

			if (!m_results)
				m_results = std::make_unique<final_result_type>();

			m_results->emplace_back(std::move(result_img));
		}
	}

	/// get final result (list of reassembled cv::Mat imgs, aged oldest to youngest)
	virtual std::unique_ptr<final_result_type> GetFinalResult() override
	{
		return std::move(m_results);
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

	/// store image fragments until they are ready to be used
	std::vector<std::list<token_type>> m_fragments{};
	/// store assembled images
	std::unique_ptr<final_result_type> m_results{};
};


#endif //header guard
















