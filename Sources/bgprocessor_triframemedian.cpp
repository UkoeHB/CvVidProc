// implementation of TriframeMedian background processing algorithm

//local headers
#include "background_processor.h"
#include "bgprocessor_triframemedian.h"
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <memory>


using TFM_T = BackgroundProcessor<TriframeMedian, cv::Mat>;

bool set_triframe_median(std::array<std::vector<unsigned char>, 3> &triframe)
{
	if (triframe[0].size() == 0 || !(triframe[0].size() == triframe[1].size() &&
									triframe[0].size() == triframe[2].size()))
	{
		std::cerr << "Unexpected frame size mismatch!\n";
		return false;
	}

	// set median of all indices
	for (std::size_t index{0}; index < triframe[0].size(); ++index)
	{
		if (triframe[0][index] < triframe[1][index] &&
			triframe[0][index] < triframe[2][index])
		{
			if (triframe[1][index] < triframe[2][index])
				triframe[0][index] = triframe[1][index];
			else
				triframe[0][index] = triframe[2][index];
		}

		if (triframe[0][index] > triframe[1][index] &&
			triframe[0][index] > triframe[2][index])
		{
			if (triframe[1][index] > triframe[2][index])
				triframe[0][index] = triframe[1][index];
			else
				triframe[0][index] = triframe[2][index];
		}
	}

	return true;
}

void TFM_T::Insert(std::unique_ptr<cv::Mat> new_element)
{
	// leave if reached the end of the video or frame is corrupted
	if (!new_element || !new_element->data || new_element->empty())
		return;

	// collect frame info from first frame
	if (m_frames_processed == 0)
	{
		m_frame_rows_count = new_element->rows;
		m_frame_channel_count = new_element->channels();
	}

	// add frame to triframe
	assert(m_triframe_position < 3);
	
	cv_mat_to_std_vector_uchar((*new_element).clone(), m_triframe[m_triframe_position]);

	++m_triframe_position;

	// consume frames when triframe is full
	if (m_triframe_position == 3)
	{
		// after the first pass through the frame at index 0 will be the background median
		m_triframe_position = 1;

		// get median of triframe
		set_triframe_median(m_triframe);
	}

	m_frames_processed++;
}

cv::Mat TFM_T::GetResult() const
{
	// convert vector to Mat image
	cv::Mat result_frame{};
	cv_mat_from_std_vector_uchar(result_frame, m_triframe[0], m_frame_rows_count, m_frame_channel_count);

	return result_frame;
}















