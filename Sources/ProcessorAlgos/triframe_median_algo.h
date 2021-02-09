// filters arrays of uchars by getting median of 3 arrays, then rolling that median into next two arrays for a new median, etc.
// - intended for computing the background image of a statically-positioned video recording

#ifndef TRIFRAME_MEDIAN_ALGO_098765_H
#define TRIFRAME_MEDIAN_ALGO_098765_H

//local headers
#include "token_processor_algo_base.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <array>
#include <memory>
#include <vector>


/// processor algorithm type declaration
class TriframeMedianAlgo;

/// not strictly necessary to specialize this, just doing so for clarity
template<>
struct TokenProcessorPack<TriframeMedianAlgo> final
{};

////
// implementation for algorithm: triframe median
// passes over cv::Mat sequence and updates a 'median' value every 2 elements
///
class TriframeMedianAlgo final : public TokenProcessorAlgoBase<TriframeMedianAlgo, cv::Mat, cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	TriframeMedianAlgo() = delete;

	/// normal constructor
	TriframeMedianAlgo(TokenProcessorPack<TriframeMedianAlgo> processor_pack) :
		TokenProcessorAlgoBase{std::move(processor_pack)}
	{}

	/// copy constructor: disabled
	TriframeMedianAlgo(const TriframeMedianAlgo&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	TriframeMedianAlgo& operator=(const TriframeMedianAlgo&) = delete;
	TriframeMedianAlgo& operator=(const TriframeMedianAlgo&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<token_type> new_element) override;

	/// get the processing result
	virtual std::unique_ptr<result_type> TryGetResult() override;

	/// get notified there are no more elements
	virtual void NotifyNoMoreTokens() override { m_done_processing = true; }

private:
//member variables
	/// number of pixel rows in frame Mat
	int m_frame_rows_count{0};
	/// number of channels in each frame Mat
	int m_frame_channel_count{0};

	/// number of frames processed
	int m_frames_processed{0};
	/// current location in triframe
	int m_triframe_position{0};
	/// triframe for processing median frame
	std::array<std::vector<unsigned char>, 3> m_triframe{};

	/// no more frames will be inserted
	bool m_done_processing{false};
};


/// set first vector in trio to element-wise median of three vectors
bool set_triframe_median(std::array<std::vector<unsigned char>, 3> &triframe);


#endif //header guard


















