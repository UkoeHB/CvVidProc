// background processor template for managing an image background processing algorithm

#ifndef BGPROCESSOR_TRIFRAMEMEDIAN_H
#define BGPROCESSOR_TRIFRAMEMEDIAN_H

//local headers
#include "background_processor.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <array>
#include <memory>
#include <vector>


/// set first vector in trio to element-wise median of three vectors
bool set_triframe_median(std::array<std::vector<unsigned char>, 3> &triframe);

/// processor type
struct TriframeMedian final
{};

/// not strictly necessary to specialize this, just doing so for clarity
template<>
struct BgProcessorPack<TriframeMedian> final
{};

////
// unimplemented sample interface (this only works when specialized)
// it's best to inherit from the same base for all specializations
///
template<>
class BackgroundProcessor<TriframeMedian, cv::Mat> final : public BackgroundProcessorBase<TriframeMedian, cv::Mat>
{
public:
//constructors
	/// default constructor: disabled
	BackgroundProcessor() = delete;

	/// normal constructor
	BackgroundProcessor(const BgProcessorPack<TriframeMedian> &processor_pack) : BackgroundProcessorBase<TriframeMedian, cv::Mat>{processor_pack}
	{}

	/// copy constructor: disabled
	BackgroundProcessor(const BackgroundProcessor&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	BackgroundProcessor& operator=(const BackgroundProcessor&) = delete;
	BackgroundProcessor& operator=(const BackgroundProcessor&) const = delete;

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



















