// opencv utility functions

//local headers
#include "cv_util.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers
#include <array>
#include <vector>


struct BorderedChunk
{
	/// x-coord of chunk within original 2-d matrix
	int corner_x{};
	/// y-coord of chunk within original 2-d matrix
	int corner_y{};
	/// width of chunk
	int final_width{};
	/// height of chunk
	int final_height{};
	/// x-coord of pre-buffer chunk within original 2-d matrix
	int original_x{};
	/// y-coord of pre-buffer chunk within original 2-d matrix
	int original_y{};
	/// width of pre-buffer chunk
	int original_width{};
	/// height of pre-buffer chunk
	int original_height{};
};

bool get_bordered_chunks(
	std::vector<BorderedChunk> &ret,
	const int original_width,
	const int original_height,
	const int col_divisor,
	const int row_divisor,
	int horizontal_buffer_pixels/* = 0*/,
	int vertical_buffer_pixels/* = 0*/)
{
	// validate inputs
	if (ret.size() > 0 ||
		original_width <= 0 || 
		original_height <= 0 || 
		col_divisor <= 0 || 
		row_divisor <= 0 || 
		horizontal_buffer_pixels < 0 || 
		vertical_buffer_pixels < 0)
		return false;

	// columns and rows of chunks
	int new_col_width = original_width / col_divisor;
	int new_row_height = original_height / row_divisor;

	int col_remainder = original_width % col_divisor;
	int row_remainder = original_height % row_divisor;

	// define all chunks
	// note: point (0,0) is the upper left corner of the chunk by convention
	int num_cols{0};
	int corner_x{};
	int corner_y{};
	int chunk_width{};
	int chunk_height{};
	int prebuffer_chunk_width{};
	int prebuffer_chunk_height{};
	for (int x_pos{0}; x_pos < original_width; x_pos += new_col_width)
	{
		// left-hand x-coord
		corner_x = x_pos - horizontal_buffer_pixels;

		if (corner_x < 0)
			corner_x = 0;

		// chunk width
		prebuffer_chunk_width = new_col_width;

		if (num_cols == col_divisor - 1)
			prebuffer_chunk_width += col_remainder;

		// right-hand x-coordinate
		chunk_width = x_pos + prebuffer_chunk_width + horizontal_buffer_pixels;		

		if (chunk_width > original_width)
			chunk_width = original_width;

		// get chunk width from difference between x-coords
		chunk_width -= corner_x;

		int num_rows{0};
		int temp_rows{new_row_height};
		for (int y_pos{0}; y_pos < original_height; y_pos += temp_rows)
		{
			// upper y-coord
			corner_y = y_pos - vertical_buffer_pixels;

			if (corner_y < 0)
				corner_y = 0;

			// chunk height
			prebuffer_chunk_height = new_row_height;

			if (num_rows == row_divisor - 1)
				prebuffer_chunk_height += row_remainder;

			// lower y-coordinate
			chunk_height = y_pos + prebuffer_chunk_height + vertical_buffer_pixels;

			if (chunk_height > original_height)
				chunk_height = original_height;

			// get chunk height from difference between y-coords
			chunk_height -= corner_y;

			ret.emplace_back(BorderedChunk{
				corner_x,
				corner_y,
				chunk_width,
				chunk_height,
				x_pos,
				y_pos,
				prebuffer_chunk_width,
				prebuffer_chunk_height
			});

			++num_rows;
		}

		++num_cols;
	}

	return true;
}


bool cv_mat_to_chunks(
		const cv::Mat &mat_input,
		std::vector<std::unique_ptr<cv::Mat>> &chunks_output,
		const int col_divisor,
		const int row_divisor,
		int horizontal_buffer_pixels/* = 0*/,
		int vertical_buffer_pixels/* = 0*/)
{
	// https://answers.opencv.org/question/53694/divide-an-image-into-lower-regions/
	// check that input Mat has content
	if (!mat_input.data || mat_input.empty())
		return false;

	// must be more than zero chunks
	if (!row_divisor || !col_divisor)
		return false;

	// only legal pixel buffers
	if (horizontal_buffer_pixels < 0)
		horizontal_buffer_pixels = 0;

	if (vertical_buffer_pixels < 0)
		vertical_buffer_pixels = 0;

	// columns and rows of Mat chunks
	int new_col_width = mat_input.cols / col_divisor;
	int new_row_height = mat_input.rows / row_divisor;

	int col_remainder = mat_input.cols % col_divisor;
	int row_remainder = mat_input.rows % row_divisor;

	// create all the Mat chunks by copying from the original Mat
	// note: point (0,0) is the upper left corner of the image by convention
	int num_cols{0};
	int corner_x{};
	int corner_y{};
	int chunk_width{};
	int chunk_height{};
	for (int x_pos{0}; x_pos < mat_input.cols; x_pos += new_col_width)
	{
		// left-hand x-coord
		corner_x = x_pos - horizontal_buffer_pixels;

		if (corner_x < 0)
			corner_x = 0;

		// right-hand x-coordinate
		chunk_width = x_pos + new_col_width + horizontal_buffer_pixels;

		if (num_cols == col_divisor - 1)
			chunk_width += col_remainder;

		if (chunk_width > mat_input.cols)
			chunk_width = mat_input.cols;

		// get chunk width from difference between x-coords
		chunk_width -= corner_x;

		int num_rows{0};
		int temp_rows{new_row_height};
		for (int y_pos{0}; y_pos < mat_input.rows; y_pos += temp_rows)
		{
			// upper y-coord
			corner_y = y_pos - vertical_buffer_pixels;

			if (corner_y < 0)
				corner_y = 0;

			// lower y-coordinate
			chunk_height = y_pos + new_row_height + vertical_buffer_pixels;

			if (num_rows == row_divisor - 1)
				chunk_height += row_remainder;

			if (chunk_height > mat_input.rows)
				chunk_height = mat_input.rows;

			// get chunk height from difference between y-coords
			chunk_height -= corner_y;

			// get rectangle from original Mat and clone (copy) it
			chunks_output.emplace_back(std::make_unique<cv::Mat>(mat_input(cv::Rect(corner_x, corner_y, chunk_width, chunk_height)).clone()));

			++num_rows;
		}

		++num_cols;
	}

	return true;
}

bool cv_mat_from_chunks(
	cv::Mat &mat_output,
	const std::vector<cv::Mat> &chunks_input,
	const int col_divisor,
	const int row_divisor,
	const int original_width,
	const int original_height,
	int horizontal_buffer_pixels/* = 0*/,
	int vertical_buffer_pixels/* = 0*/)
{
	// must be expected number of chunks (and nonzero)
	if (!(row_divisor*col_divisor) || row_divisor*col_divisor != chunks_input.size())
		return false;

	if (!chunks_input.front().data ||
		chunks_input.front().empty() ||
		!chunks_input.back().data ||
		chunks_input.back().empty())
		return false;

	int chunk_cols = chunks_input.front().cols;
	int chunk_rows = chunks_input.front().rows;

	int col_remainder = chunks_input.back().cols - chunk_cols;
	int row_remainder = chunks_input.back().rows - chunk_rows;

	// initialize temp Mat with expected dimensions
	cv::Mat temp_output(chunk_rows*row_divisor + row_remainder, chunk_cols*col_divisor + col_remainder, chunks_input.front().type());

	auto mat_element_it = chunks_input.begin();

	for (int col{0}; col < col_divisor; col++)
	{
		// collect the column elements into solid columns
		for (int row{0}; row < row_divisor; row++)
		{
			if (!mat_element_it->data ||
				mat_element_it->empty() ||
				mat_element_it->cols != chunk_cols + (col == col_divisor - 1 ? col_remainder : 0) ||
				mat_element_it->rows != chunk_rows + (row == row_divisor - 1 ? row_remainder : 0))
				return false;

			// https://stackoverflow.com/questions/33239669/opencv-how-to-merge-two-images?noredirect=1&lq=1
			// copy chunk directly into temp Mat
			(*mat_element_it).copyTo(temp_output(cv::Rect(
					col*chunk_cols,
					row*chunk_rows,
					mat_element_it->cols,
					mat_element_it->rows
				)));

			++mat_element_it;
		}
	}

	mat_output = std::move(temp_output);

	return true;
}

bool cv_mat_to_std_vector_uchar(const cv::Mat &mat_input, std::vector<unsigned char> &vec_output)
{
	// check that input Mat has content
	if (!mat_input.data || mat_input.empty())
		return false;

	// make sure the Mat is an unsigned char
	// http://ninghang.blogspot.com/2012/11/list-of-mat-type-in-opencv.html
	assert(mat_input.type() == CV_8UC1 ||
			mat_input.type() == CV_8UC2 ||
			mat_input.type() == CV_8UC3 || 
			mat_input.type() == CV_8UC4);

	vec_output.clear();

	// https://stackoverflow.com/questions/26681713/convert-mat-to-array-vector-in-opencv
	// performs copies of the input data
	if (mat_input.isContinuous())
	{
		vec_output.assign(mat_input.data, mat_input.data + mat_input.total()*mat_input.channels());
	}
	else
	{
		for (int i = 0; i < mat_input.rows; ++i) {
		    vec_output.insert(vec_output.end(), mat_input.ptr<unsigned char>(i), mat_input.ptr<unsigned char>(i) + mat_input.cols*mat_input.channels());
		}
	}

	return true;
}

bool cv_mat_from_std_vector_uchar(cv::Mat &mat_output, const std::vector<unsigned char> &vec_input, const int rows, const int channels)
{
	// https://answers.opencv.org/question/81831/convert-stdvectordouble-to-mat-and-show-the-image/
	// convert the vector to a Mat then reshape into expected rows/columns/channels (columns are inferred)
	mat_output = cv::Mat{vec_input, true}.reshape(channels, rows);

	// note: for unsigned chars (at least), the Mat.type() will be recovered safely without needing to manually convert it

	return true;
}

std::string get_fourcc_code_str(int x)
{
	// https://answers.opencv.org/question/77558/get-fourcc-after-openning-a-video-file/
	return cv::format("%c%c%c%c", x & 255, (x >> 8) & 255, (x >> 16) & 255, (x >> 24) & 255);
}














