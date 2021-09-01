// opencv utility functions

#ifndef CV_UTIL_579393993_H
#define CV_UTIL_579393993_H

//local headers

//third party headers
#include <opencv2/opencv.hpp>   //for video manipulation (mainly)

//standard headers
#include <vector>

//forward declarations


////
// split Mat into row_divisor*col_divisor chunks
// output chunks are laid out [col1 elements][col2 elements][col3 elements] in vector
// last chunks in each row or column will be larger than others if Mat dimensions don't divide perfectly
// chunk-to-final-mat alignment is defined by get_bordered_chunks()
///
bool cv_mat_to_chunks(
	const cv::Mat &mat_input,
	std::vector<std::unique_ptr<cv::Mat>> &chunks_output,
	const int col_divisor,
	const int row_divisor,
	int horizontal_buffer_pixels = 0,
	int vertical_buffer_pixels = 0);

////
// reassemble Mat from Mat chunks
// assumes input chunks are laid out [col1 elements][col2 elements][col3 elements] in vector
// and only the last chunks in each column and row might be larger then the rest
// chunk-to-final-mat alignment is defined by get_bordered_chunks()
///
bool cv_mat_from_chunks(
	cv::Mat &mat_output,
	const std::vector<cv::Mat> &chunks_input,
	const int col_divisor,
	const int row_divisor,
	const int final_width,
	const int final_height,
	int horizontal_buffer_pixels = 0,
	int vertical_buffer_pixels = 0);

/// convert Mat to std::vector<unsigned char>
bool cv_mat_to_std_vector_uchar(const cv::Mat &mat_input, std::vector<unsigned char> &vec_output);

/// convert Mat from std::vector<unsigned char>
bool cv_mat_from_std_vector_uchar(cv::Mat &mat_output, const std::vector<unsigned char> &vec_input, const int rows, const int channels);

/// get fourcc code from integer (video format code used by OpenCV)
std::string get_fourcc_code_str(int x);




#endif	//header guard


















