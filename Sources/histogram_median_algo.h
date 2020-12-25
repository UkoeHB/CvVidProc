// computes element-wise median of cv::Mat sequence by collecting histograms for each element
// - intended for computing the background image of a statically-positioned video recording

#ifndef HISTOGRAM_MEDIAN_ALGO_5776890_H
#define HISTOGRAM_MEDIAN_ALGO_5776890_H

//local headers
#include "token_processor.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>


/// processor algorithm type
template <typename T>
struct HistogramMedianAlgo final
{
	using token_type = cv::Mat;
	using result_type = cv::Mat;
};

/// main types to use for interacting with the histogram median algorithm
/// - prefer larger types if dealing with more elements, otherwise the median will be less accurate
/// - but beware the RAM cost, as there are 256 histogram elements per cv::Mat element
/// 	- a 1280x512 image has 655k pixels, which is 2mill cv::Mat elements for all the channels, or 500 MB per histogram byte
using HistogramMedianAlgo8 = HistogramMedianAlgo<unsigned char>;	//255 elements
using HistogramMedianAlgo16 = HistogramMedianAlgo<std::uint16_t>;	//65525 elements
using HistogramMedianAlgo32 = HistogramMedianAlgo<std::uint32_t>;	//4294967295 elements

template <typename T>
struct TokenProcessorPack<HistogramMedianAlgo<T>> final
{};

////
// implementation for algorithm: histogram median
// collects 
///
template <typename T>
class TokenProcessor<HistogramMedianAlgo<T>> final : public TokenProcessorBase<HistogramMedianAlgo<T>>
{
public:
//constructors
	/// default constructor: disabled
	TokenProcessor() = delete;

	/// normal constructor
	TokenProcessor(TokenProcessorPack<HistogramMedianAlgo<T>> processor_pack) : TokenProcessorBase<HistogramMedianAlgo<T>>{std::move(processor_pack)}
	{
		static_assert(std::is_unsigned<T>::value, "HistogramMedianAlgo only works with unsigned integrals for histogram elements!");
	}

	/// copy constructor: disabled
	TokenProcessor(const TokenProcessor&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	TokenProcessor& operator=(const TokenProcessor&) = delete;
	TokenProcessor& operator=(const TokenProcessor&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<cv::Mat> new_mat) override
	{
		// leave if reached the end of the video or frame is corrupted
		if (!new_mat || !new_mat->data || new_mat->empty())
			return;

		// collect frame info from first frame
		if (m_frames_processed == 0)
		{
			m_frame_rows_count = new_mat->rows;
			m_frame_channel_count = new_mat->channels();
		}

		// convert frame to vector
		std::vector<unsigned char> frame_as_vec{};
		cv_mat_to_std_vector_uchar((*new_mat).clone(), frame_as_vec);

		// increment histograms
		ConsumeVector(frame_as_vec);

		m_frames_processed++;
	}

	/// get the processing result
	virtual bool TryGetResult(std::unique_ptr<cv::Mat> &return_result) override
	{
		// for triframe median, only get a result if no more frames will be sent in
		if (!m_done_processing)
			return false;

		// collect histogram results
		std::vector<unsigned char> result_vec{MedianFromHistograms()};

		// convert vector to Mat image
		cv::Mat result_frame{};
		cv_mat_from_std_vector_uchar(result_frame, result_vec, m_frame_rows_count, m_frame_channel_count);

		return_result = std::make_unique<cv::Mat>(std::move(result_frame));

		return true;
	}

	/// get notified there are no more elements
	virtual void NotifyNoMoreTokens() override { m_done_processing = true; }

	/// increment histograms
	void ConsumeVector(const std::vector<unsigned char> &new_elements)
	{
		// initialize histograms for first element
		if (m_frames_processed == 0)
		{
			assert(new_elements.size() > 0);

			std::vector<T> element_histograms{};
			unsigned char max_char{static_cast<unsigned char>(-1)};
			element_histograms.resize(max_char, T{0});
			m_histograms.resize(new_elements.size(), element_histograms);

			// check that it worked
			assert(m_histograms[0].size() == static_cast<std::size_t>(max_char));
		}

		// increment all the histograms
		for (std::size_t element_index{0}; element_index < m_histograms.size(); element_index++)
		{
			// only increment histogram if it won't cause roll-over
			if (m_histograms[element_index][static_cast<std::size_t>(new_elements[element_index])] != T{static_cast<T>(-1)})
			{
				m_histograms[element_index][static_cast<std::size_t>(new_elements[element_index])]++;
			}
		}
	}

	/// collect median from histograms
	std::vector<unsigned char> MedianFromHistograms()
	{
		std::vector<unsigned char> return_vec{};
		return_vec.resize(m_histograms.size());
		unsigned char max_char{static_cast<unsigned char>(-1)};
		unsigned long accumulator_cap{static_cast<unsigned long>(m_frames_processed)};

		assert(m_histograms.size() > 0);

		for (std::size_t element_index{0}; element_index < m_histograms.size(); element_index++)
		{
			unsigned long accumulator{0};
			std::size_t halfway_index{0};

			// find the histogram index that sits in the middle of all items added
			for (std::size_t histogram_index{0}; histogram_index < static_cast<std::size_t>(max_char); histogram_index++)
			{
				accumulator += static_cast<unsigned long>(m_histograms[element_index][histogram_index]);

				if (!halfway_index && accumulator > accumulator_cap/2)
					halfway_index = histogram_index;
			}

			// if a histogram index reached its limit, then maybe all items aren't accounted for, so backtrack
			if (accumulator != accumulator_cap)
			{
				// set temp cap to actual number of items counted
				unsigned long temp_cap{accumulator};

				for (std::size_t histogram_index{halfway_index}; histogram_index != std::size_t{static_cast<std::size_t>(-1)}; histogram_index--)
				{
					accumulator -= static_cast<unsigned long>(m_histograms[element_index][histogram_index]);

					// use the histogram index above the halfway mark
					if (accumulator < temp_cap/2)
						break;

					halfway_index--;
				}
			}

			return_vec[element_index] = static_cast<unsigned char>(halfway_index);
		}

		return return_vec;
	}

private:
//member variables
	/// number of pixel rows in frame Mat
	int m_frame_rows_count{0};
	/// number of channels in each frame Mat
	int m_frame_channel_count{0};

	/// number of frames processed
	int m_frames_processed{0};

	/// histograms for processing median of each element; parent vector is all image elements, child vector is histogram
	std::vector<std::vector<T>> m_histograms{};

	/// no more frames will be inserted
	bool m_done_processing{false};
};


#endif //header guard



















