// opencv utility functions

//local headers
#include "cv_util.h"
#include "exception_assert.h"

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
	int chunk_width{};
	/// height of chunk
	int chunk_height{};
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
	for (int x_pos{0}; x_pos < original_width - col_remainder; x_pos += new_col_width)
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
		for (int y_pos{0}; y_pos < original_height - row_remainder; y_pos += temp_rows)
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

	// sanity check
	if (chunks_output.size() != 0)
		return false;

	// figure out location and size of all chunks within original 2-d Mat matrix
	// if this fails then inputs are invalid
	std::vector<BorderedChunk> chunks{};
	if (!get_bordered_chunks(chunks, mat_input.cols, mat_input.rows, col_divisor, row_divisor, horizontal_buffer_pixels, vertical_buffer_pixels))
		return false;

	// sanity check
	if (chunks.size() != col_divisor*row_divisor)
		return false;

	// create all the actual chunks
	chunks_output.reserve(chunks.size());

	for (const auto& chunk : chunks)
	{
		chunks_output.emplace_back(std::make_unique<cv::Mat>(mat_input(cv::Rect(chunk.corner_x, chunk.corner_y, chunk.chunk_width, chunk.chunk_height)).clone()));
	}

	return true;
}

bool cv_mat_from_chunks(
	cv::Mat &mat_output,
	const std::vector<cv::Mat> &chunks_input,
	const int col_divisor,
	const int row_divisor,
	const int final_width,
	const int final_height,
	int horizontal_buffer_pixels/* = 0*/,
	int vertical_buffer_pixels/* = 0*/)
{
	// must be expected number of chunks (and nonzero)
	if (!(row_divisor*col_divisor) || row_divisor*col_divisor != chunks_input.size())
		return false;

	// figure out location and size of all chunks within original 2-d Mat matrix
	// if this fails then inputs are invalid
	std::vector<BorderedChunk> chunks{};
	if (!get_bordered_chunks(chunks, final_width, final_height, col_divisor, row_divisor, horizontal_buffer_pixels, vertical_buffer_pixels))
		return false;

	// sanity check
	if (chunks.size() != chunks_input.size())
		return false;

	// initialize temp Mat with expected dimensions
	if (!chunks_input.front().data ||
		chunks_input.front().empty())
		return false;

	cv::Mat temp_output(final_height, final_width, chunks_input.front().type());

	// https://stackoverflow.com/questions/33239669/opencv-how-to-merge-two-images?noredirect=1&lq=1
	// copy chunks directly into temp Mat
	for (std::size_t chunk_index{0}; chunk_index < chunks.size(); ++chunk_index)
	{
		if (!chunks_input[chunk_index].data ||
			chunks_input[chunk_index].empty())
			return false;

		// copy non-buffered component of input chunk into intended location in output Mat
		// https://stackoverflow.com/questions/33239669/opencv-how-to-merge-two-images?noredirect=1&lq=1
		const auto &input_chunk = chunks_input[chunk_index](cv::Rect(
				chunks[chunk_index].original_x - chunks[chunk_index].corner_x,
				chunks[chunk_index].original_y - chunks[chunk_index].corner_y,
				chunks[chunk_index].original_width,
				chunks[chunk_index].original_height
			));

		input_chunk.copyTo(temp_output(cv::Rect(
				chunks[chunk_index].original_x,
				chunks[chunk_index].original_y,
				chunks[chunk_index].original_width,
				chunks[chunk_index].original_height
			)));
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
	EXCEPTION_ASSERT(mat_input.type() == CV_8UC1 ||
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
		for (int i = 0; i < mat_input.rows; ++i)
		{
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














