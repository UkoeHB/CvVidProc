// highlights bubbles in an image

#ifndef HIGHLIGHT_BUBBLES_ALGO_6787878_H
#define HIGHLIGHT_BUBBLES_ALGO_6787878_H

//local headers
#include "token_processor_algo_base.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <memory>
#include <vector>


/// processor algorithm type declaration
class HighlightBubblesAlgo;

/// specialization of variable pack
template <>
struct TokenProcessorPack<HighlightBubblesAlgo> final
{
	cv::Mat background{};
	cv::Mat struct_element{};
	const int threshold{};
	const int threshold_lo{};
	const int threshold_hi{};
	const int min_size_hyst{};
	const int min_size_threshold{};
	const int width_border{};
};

////
// C++ implementation for algorithm: highlight_bubble_hyst_thresh()
// obtains a cv::Mat image and uses heuristics to process the image so bubble-like objects are highlighted
///
class HighlightBubblesAlgo final : public TokenProcessorAlgoBase<HighlightBubblesAlgo, cv::Mat, cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	HighlightBubblesAlgo() = delete;

	/// normal constructor
	HighlightBubblesAlgo(TokenProcessorPack<HighlightBubblesAlgo> processor_pack) : TokenProcessorAlgoBase{std::move(processor_pack)}
	{}

	/// copy constructor: disabled
	HighlightBubblesAlgo(const HighlightBubblesAlgo&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	HighlightBubblesAlgo& operator=(const HighlightBubblesAlgo&) = delete;
	HighlightBubblesAlgo& operator=(const HighlightBubblesAlgo&) const = delete;

//member functions
	/// insert an element to be processed
	/// note: overrides result if there already is one
	virtual void Insert(std::unique_ptr<cv::Mat> in_mat) override
	{
		// leave if image not available
		if (!in_mat || !in_mat->data || in_mat->empty())
			return;

		HighlightBubbles(*in_mat);

		m_result = std::move(in_mat);
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
		// do nothing (each token processed is independent of the previous)
	}

	/// C++ implementation of highlight_bubbles()
	void HighlightBubbles(cv::Mat &frame);

	/// C++ implementation of thresh_im()
	cv::Mat thresh_im(cv::Mat &image, const int threshold);

	/// C++ implementation of hysteresis_threshold()
	cv::Mat hysteresis_threshold(cv::Mat &image, const int threshold_lo, const int threshold_hi);

	/// C++ implementation of remove_small_objects_find()
	void remove_small_objects_find(cv::Mat &image, const int min_size_threshold);

	/// C++ implementation of fill_holes()
	void fill_holes(cv::Mat &image);

	/// C++ implementation of frame_and_fill()
	void frame_and_fill(cv::Mat &image, const int width_border);


private:
//member variables
	/// store result in anticipation of future requests
	std::unique_ptr<cv::Mat> m_result{};
};


#endif //header guard





















