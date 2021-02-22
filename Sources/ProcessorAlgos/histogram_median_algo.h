// computes element-wise median of cv::Mat sequence by collecting histograms for each element
// - intended for computing the background image of a statically-positioned video recording

#ifndef HISTOGRAM_MEDIAN_ALGO_5776890_H
#define HISTOGRAM_MEDIAN_ALGO_5776890_H

//local headers
#include "token_processor_algo.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>


/// processor algorithm type declaration
template <typename T>
class HistogramMedianAlgo;

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
// collects cv::Mat frames, increments histograms for each pixel value, then gets median of each histogram for element-wise cv::Mat median
///
template <typename T>
class HistogramMedianAlgo final : public TokenProcessorAlgo<HistogramMedianAlgo<T>, cv::Mat, cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	HistogramMedianAlgo() = delete;

	/// normal constructor
	HistogramMedianAlgo(TokenProcessorPack<HistogramMedianAlgo<T>> processor_pack) :
		TokenProcessorAlgo<HistogramMedianAlgo<T>, cv::Mat, cv::Mat>{std::move(processor_pack)}
	{
		static_assert(std::is_unsigned<T>::value, "HistogramMedianAlgo only works with unsigned integrals for histogram elements!");
	}

	/// copy constructor: disabled
	HistogramMedianAlgo(const HistogramMedianAlgo&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	HistogramMedianAlgo& operator=(const HistogramMedianAlgo&) = delete;
	HistogramMedianAlgo& operator=(const HistogramMedianAlgo&) const = delete;

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
	virtual std::unique_ptr<cv::Mat> TryGetResult() override
	{
		// get result if there is one
		if (m_result)
			return std::move(m_result);
		else
			return nullptr;
	}

	/// get notified there are no more elements
	virtual void NotifyNoMoreTokens() override
	{ 
		// no more tokens, so set the result
		SetResult();

		// reset number of frames processed
		m_frames_processed = 0;
	}

	/// report if there is a result to get
	virtual bool HasResults() override
	{
		return static_cast<bool>(m_result);
	}

	/// increment histograms
	void ConsumeVector(const std::vector<unsigned char> &new_elements)
	{
		// initialize histograms for first element
		if (m_frames_processed == 0)
		{
			assert(new_elements.size() > 0);

			std::vector<T> frequency_map{};
			std::size_t max_uchar{static_cast<unsigned char>(-1)};
			frequency_map.resize(new_elements.size(), T{0});
			m_histograms.resize(max_uchar + 1, frequency_map);

			// check that it worked
			assert(m_histograms[0].size() == new_elements.size());
		}

		// increment all the histograms
		for (std::size_t element_index{0}; element_index < m_histograms[0].size(); element_index++)
		{
			// only increment histogram if it won't cause roll-over
			if (m_histograms[static_cast<std::size_t>(new_elements[element_index])][element_index] != static_cast<T>(-1))
			{
				m_histograms[static_cast<std::size_t>(new_elements[element_index])][element_index]++;
			}
		}
	}

	/// collect median from histograms
	std::vector<unsigned char> MedianFromHistograms()
	{
		assert(m_histograms.size() > 0);
		assert(m_histograms[0].size() > 0);

		std::vector<unsigned char> return_vec{};
		return_vec.resize(m_histograms[0].size());
		std::size_t max_uchar{static_cast<unsigned char>(-1)};
		unsigned long accumulator_cap{static_cast<unsigned long>(m_frames_processed)};

		for (std::size_t element_index{0}; element_index < m_histograms[0].size(); element_index++)
		{
			unsigned long accumulator{0};
			std::size_t halfway_index{max_uchar};

			// find the histogram index that sits in the middle of all items added
			for (std::size_t histogram_index{0}; histogram_index < max_uchar + 1; histogram_index++)
			{
				accumulator += static_cast<unsigned long>(m_histograms[histogram_index][element_index]);

				if ((halfway_index == max_uchar) && (accumulator > accumulator_cap/2))
					halfway_index = histogram_index;
			}

			// if a histogram index reached its limit, then maybe all items aren't accounted for, so backtrack
			if (accumulator != accumulator_cap)
			{
				// set temp cap to actual number of items counted
				unsigned long temp_cap{accumulator};

				for (std::size_t histogram_index{halfway_index}; histogram_index != static_cast<std::size_t>(-1); histogram_index--)
				{
					accumulator -= static_cast<unsigned long>(m_histograms[histogram_index][element_index]);

					// use the histogram index above the halfway mark
					if (accumulator < temp_cap/2)
						break;

					halfway_index--;
				}
			}

			// sanity check
			assert(halfway_index < max_uchar + 1);

			return_vec[element_index] = static_cast<unsigned char>(halfway_index);
		}

		return return_vec;
	}

	void SetResult()
	{
		// collect histogram results
		std::vector<unsigned char> result_vec{MedianFromHistograms()};

		// convert vector to Mat image
		cv::Mat result_frame{};
		cv_mat_from_std_vector_uchar(result_frame, result_vec, m_frame_rows_count, m_frame_channel_count);

		// set the result
		m_result = std::make_unique<cv::Mat>(std::move(result_frame));
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
	/// store result in anticipation of future requests
	std::unique_ptr<cv::Mat> m_result{};
};


#endif //header guard



















