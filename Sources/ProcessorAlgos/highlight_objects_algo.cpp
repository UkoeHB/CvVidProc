// highlights objects in an image

//local headers
#include "highlight_objects_algo.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <iostream>
#include <memory>
#include <vector>


void HighlightObjectsAlgo::HighlightObjects(cv::Mat &frame)
{
	/*
		first performs a low threshold and
		high minimum size to get faint, large objects, and then performs a higher
		hysteresis threshold with a low minimum size to get distinct, small
		objects.
	*/

	//im_diff = cv2.absdiff(bkgd, frame)
	cv::Mat im_diff{cv::Size{frame.cols, frame.rows}, frame.type()};
	cv::absdiff(m_pack.background, frame, im_diff);

	///////////////////// THRESHOLD AND HIGH MIN SIZE /////////////////////
	// thresholds image to become black-and-white
	//thresh_bw_1 = thresh_im(im_diff, th)
	cv::Mat thresh_bw_1{ThresholdImage(im_diff, m_pack.threshold)};

	// smooths out thresholded image
	//closed_bw_1 = cv2.morphologyEx(thresh_bw_1, cv2.MORPH_OPEN, selem)
	cv::morphologyEx(thresh_bw_1, thresh_bw_1, cv::MorphTypes::MORPH_OPEN, m_pack.struct_element);

	// removes small objects
	//bubble_bw_1 = remove_small_objects(closed_bw_1, min_size_th)
	RemoveSmallObjects(thresh_bw_1, m_pack.min_size_threshold);

	// fills enclosed holes with white, but leaves open holes black
	//bubble_1 = basic.fill_holes(bubble_bw_1)
	FillHoles(thresh_bw_1);

	///////////////////// HYSTERESIS THRESHOLD AND LOW MIN SIZE /////////////////////
	// thresholds image to become black-and-white
	// thresh_bw_2 = skimage.filters.apply_hysteresis_threshold(\
	//					 im_diff, th_lo, th_hi)
	//thresh_bw_2 = hysteresis_threshold(im_diff, th_lo, th_hi)
	cv::Mat thresh_bw_2{ThresholdImageWithHysteresis(im_diff, m_pack.threshold_lo, m_pack.threshold_hi)};

	//thresh_bw_2 = basic.cvify(thresh_bw_2)
	// not needed

	// smooths out thresholded image
	//closed_bw_2 = cv2.morphologyEx(thresh_bw_2, cv2.MORPH_OPEN, selem)
	cv::morphologyEx(thresh_bw_2, thresh_bw_2, cv::MorphTypes::MORPH_OPEN, m_pack.struct_element);

	// removes small objects
	//bubble_bw_2 = remove_small_objects(closed_bw_2, min_size_hyst)
	RemoveSmallObjects(thresh_bw_2, m_pack.min_size_hyst);

	// fills in holes that might be cut off at border
	//bubble_2 = frame_and_fill(bubble_part_filled, width_border)
	// why do fill_holes() before this? why not just do frame_and_fill() in the first place
	// FrameAndFill gets buggy if an object passes through the seed point (currently the origin)
	//FrameAndFill(thresh_bw_2, m_pack.width_border);
	//just fill holes instead--this may lose the center of some objects that are on the border
	FillHoles(thresh_bw_2);

	// merges images to create final image
	//bubble = np.logical_or(bubble_1, bubble_2)
	cv::bitwise_or(thresh_bw_1, thresh_bw_2, frame);
}

/// C++ implementation of thresh_im()
cv::Mat HighlightObjectsAlgo::ThresholdImage(cv::Mat &image, const int threshold)
{
	/*
		Applies a threshold to the image and returns black-and-white result.
	*/

	cv::Mat thresholded_im{};

	if (threshold == -1)
	{
		// Otsu's thresholding (automatically finds optimal thresholding value)
		//ret, thresh_im = cv2.threshold(im, 0, 255, cv2.THRESH_BINARY+cv2.THRESH_OTSU)
		// (ignores return val)
		cv::threshold(image, thresholded_im, 0, 255, cv::ThresholdTypes::THRESH_BINARY + cv::ThresholdTypes::THRESH_OTSU);
	}
	else
	{
		//ret, thresh_im = cv2.threshold(im, thresh, 255, cv2.THRESH_BINARY)
		// (ignores return val)
		cv::threshold(image, thresholded_im, threshold, 255, cv::ThresholdTypes::THRESH_BINARY);
	}

	return thresholded_im;
}

/// C++ implementation of hysteresis_threshold()
cv::Mat HighlightObjectsAlgo::ThresholdImageWithHysteresis(cv::Mat &image, const int threshold_lo, const int threshold_hi)
{
	/*
		Applies a hysteresis threshold using only OpenCV methods to replace the
		method from scikit-image, skimage.filters.hysteresis_threshold.

		Lightly adapted from:
		https://stackoverflow.com/questions/47311595/opencv-hysteresis-thresholding-implementation
	*/
	//_, thresh_upper = cv2.threshold(im, th_hi, 128, cv2.THRESH_BINARY)
	cv::Mat thresh_upper{};
	cv::threshold(image, thresh_upper, threshold_hi, 128, cv::ThresholdTypes::THRESH_BINARY);

	//_, thresh_lower = cv2.threshold(im, th_lo, 128, cv2.THRESH_BINARY)
	cv::Mat thresh_lower{};
	cv::threshold(image, thresh_lower, threshold_lo, 128, cv::ThresholdTypes::THRESH_BINARY);

	// finds the contours to get the seed from which to start the floodfill
	//_, cnts_upper, _ = cv2.findContours(thresh_upper, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
	std::vector<std::vector<cv::Point>> countours_upper{};
	cv::findContours(thresh_upper, countours_upper, cv::RetrievalModes::RETR_EXTERNAL, cv::ContourApproximationModes::CHAIN_APPROX_NONE);

	// Makes brighter the regions that contain a seed
	for (auto countour : countours_upper)
	{
		// C++ implementation
		// cv2.floodFill(threshLower, cnt[0], 255, 0, 2, 2, CV_FLOODFILL_FIXED_RANGE)
		// Python implementation
		//cv2.floodFill(thresh_lower, None, tuple(cnt[0][0]), 255, 2, 2, cv2.FLOODFILL_FIXED_RANGE)
		cv::floodFill(thresh_lower, countour[0], cv::Scalar{255}, 0, 2, 2, cv::FloodFillFlags::FLOODFILL_FIXED_RANGE);
	}

	// thresholds the image again to make black the unfilled regions
	//_, im_thresh = cv2.threshold(thresh_lower, 200, 255, cv2.THRESH_BINARY)
	cv::threshold(thresh_lower, thresh_lower, 200, 255, cv::ThresholdTypes::THRESH_BINARY);

	return thresh_lower;
}

void HighlightObjectsAlgo::RemoveSmallObjects(cv::Mat &image, const int min_size_threshold)
{
	/*
		Removes small objects in image with OpenCV's findContours.
		Uses OpenCV to replicate `skimage.morphology.remove_small_objects`.

		Appears to be faster than with connectedComponentsWithStats (see
		compare_props_finders.py).

		Based on response from nathancy @
		https://stackoverflow.com/questions/60033274/how-to-remove-small-object-in-image-with-python
	*/
	// Filter using contour area and remove small noise
	//_, cnts, _ = cv2.findContours(im, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	std::vector<std::vector<cv::Point>> contours{};
	cv::findContours(image, contours, cv::RetrievalModes::RETR_TREE, cv::ContourApproximationModes::CHAIN_APPROX_SIMPLE);

	std::vector<std::vector<cv::Point>> thresholded_contours{};
	thresholded_contours.reserve(contours.size());

	for (auto &contour : contours)
	{
		//area = cv2.contourArea(c)
		double area = cv::contourArea(contour);

		if (area < min_size_threshold)
		{
			//cv2.drawContours(im, [c], -1, (0,0,0), -1)
			thresholded_contours.emplace_back(std::move(contour));
		}
	}

	cv::drawContours(image, thresholded_contours, -1, cv::Scalar{0, 0, 0}, -1);

	// return by reference
}

void HighlightObjectsAlgo::FillHoles(cv::Mat &image)
{
	/*
		Fills holes in image solely using OpenCV to replace
		`fill_holes` for porting to C++.

		Based on:
		 https://learnopencv.com/filling-holes-in-an-image-using-opencv-python-c/
	*/
	// formats image for OpenCV
	//im_floodfill = cvify(im_bw)
	cv::Mat im_floodfill{image.clone()};

	// fills bkgd with white (assuming origin is contiguously connected with bkgd)
	//cv2.floodFill(im_floodfill, None, (0,0), 255)
	// (ignores return val)
	cv::floodFill(im_floodfill, cv::Point{0,0}, cv::Scalar{255});

	// inverts image (black -> white and white -> black)
	//im_inv = cv2.bitwise_not(im_floodfill)
	cv::bitwise_not(im_floodfill, im_floodfill);

	// combines inverted image with original image to fill holes
	//im_filled = (im_inv | im_bw)
	cv::bitwise_or(image, im_floodfill, image);

	// return by reference
}

void HighlightObjectsAlgo::FrameAndFill(cv::Mat &image, const int width_border)
{
	/*
		Frames image with border to fill in holes cut off at the edge. Without
		adding a frame, a hole in an object that is along the edge will not be
		viewed as a hole to be filled by fill_holes
		since a "hole" must be completely enclosed by white object pixels.
	*/
	// removes any white pixels slightly inside the border of the image
	//mask_full = np.ones([im.shape[0]-2*w, im.shape[1]-2*w])
	//mask_deframe = np.zeros_like(im)
	//mask_deframe[w:-w,w:-w] = mask_full
	// gets an image-sized mask with borders full of 0s and center full of 1s

	//mask_frame_sides = np.logical_not(mask_deframe)
	// flips 1s and 0s... (why not just make it the right way from the start?)

	// make the tops and bottoms black so only the sides are kept
	//mask_frame_sides[0:w,:] = 0
	//mask_frame_sides[-w:-1,:] = 0
	// frames sides of filled bubble image
	//im_framed = np.logical_or(im, mask_frame_sides)

	assert(image.cols > 2*width_border);
	assert(image.rows > 2*width_border);

	// instead just make a sliver image and use .copyTo()
	// make sides all white
	cv::Mat cutoff_sliver{image.rows - 2*width_border, width_border, image.type(), cv::Scalar{255}};
	cutoff_sliver.copyTo(image(cv::Rect(
		0,
		width_border,
		width_border,
		image.rows - 2*width_border
	)));

	cutoff_sliver.copyTo(image(cv::Rect(
		image.cols - width_border,  //other side
		width_border,
		width_border,
		image.rows - 2*width_border
	)));

	// fills in open space in the middle of the bubble that might not get
	// filled if bubble is on the edge of the frame (because then that
	// open space is not completely bounded)
	//im_filled = basic.fill_holes(im_framed)
	// fills holes
	FillHoles(image);

	//image = mask_im(im_filled, np.logical_not(mask_frame_sides))
	// cuts out sides
	cutoff_sliver = cv::Scalar{0};
	cutoff_sliver.copyTo(image(cv::Rect(
		0,
		width_border,
		width_border,
		image.rows - 2*width_border
	)));

	cutoff_sliver.copyTo(image(cv::Rect(
		image.cols - width_border,  //other side
		width_border,
		width_border,
		image.rows - 2*width_border
	)));

	// return by reference
}


























