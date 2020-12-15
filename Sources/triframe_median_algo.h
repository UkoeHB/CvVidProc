// background processor template for managing an image background processing algorithm

#ifndef TRIFRAME_MEDIAN_ALGO_098765_H
#define TRIFRAME_MEDIAN_ALGO_098765_H

//local headers
#include "token_processor.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <array>
#include <memory>
#include <vector>


/// set first vector in trio to element-wise median of three vectors
bool set_triframe_median(std::array<std::vector<unsigned char>, 3> &triframe);

/// processor type
struct TriframeMedianAlgo final
{};

/// not strictly necessary to specialize this, just doing so for clarity
template<>
struct TokenProcessorPack<TriframeMedianAlgo> final
{};

////
// implementation for algorithm: triframe median
// passes over cv::Mat sequence and updates a 'median' value every 2 elements
///
template<>
class TokenProcessor<TriframeMedianAlgo, cv::Mat> final : public TokenProcessorBase<TriframeMedianAlgo, cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	TokenProcessor() = delete;

	/// normal constructor
	TokenProcessor(const TokenProcessorPack<TriframeMedianAlgo> &processor_pack) : TokenProcessorBase<TriframeMedianAlgo, cv::Mat>{processor_pack}
	{}

	/// copy constructor: disabled
	TokenProcessor(const TokenProcessor&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	TokenProcessor& operator=(const TokenProcessor&) = delete;
	TokenProcessor& operator=(const TokenProcessor&) const = delete;

//member functions
	/// insert an element to be processed
	void Insert(std::unique_ptr<cv::Mat> new_element) override;

	/// get the processing result
	cv::Mat GetResult() const override;

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
};




#endif //header guard


















