// random tests

//local headers
#include "cv_vid_bg_helpers.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "project_dir_config.h"
#include "rand_tests.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

//standard headers
#include <cassert>
#include <iostream>
#include <memory>


namespace py = pybind11;
using namespace py::literals;

namespace rand_tests
{

// test bubble highlighting
void test_bubblehighlighting(cv::Mat &background_frame, const CommandLinePack &cl_pack)
{
	// for test video 'bubble_15000fps.mp4'

	// set parameters
	TokenProcessorPack<HighlightBubblesAlgo> bubbles_pack{
		background_frame,
		cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, cv::Size{4, 4}),
		14,
		7,
		16,
		20,
		20,
		5
	};

	// create algo object for processing frames (highlighting bubbles)
	HighlightBubblesAlgo highlight_bubbles{bubbles_pack};

	// get 10th frame of video
	cv::VideoCapture vid{cl_pack.vid_path};
	vid.set(cv::CAP_PROP_POS_FRAMES, 10);

	cv::Mat vid_frame{};
	vid >> vid_frame;

	// grayscale (if vid is grayscale)
	cv::Mat modified_frame{};
	cv::extractChannel(vid_frame, modified_frame, 0);

	// convert format
	std::unique_ptr<cv::Mat> frame{std::make_unique<cv::Mat>(modified_frame)};

	// insert to algo (processes the frame)
	highlight_bubbles.Insert(std::move(frame));

	// get the processed result out
	frame = highlight_bubbles.TryGetResult();

	// display the highlighted bubbles
	if (frame && frame->data && !frame->empty())
	{
		cv::imshow("Highlighted Bubbles", *frame);

		// wait for a keypress before ending
		int keypress{cv::waitKey()};
	}
	else
		std::cerr << "Bubbles frame created was malformed, unexpectedly!\n";
}

void test_embedded_python()
{
	// test python interpreter
	// create Python interpreter
	py::scoped_interpreter guard{};

	// add location of local python libs to path so they can be found
	py::module_ sys = py::module_::import("sys");
	py::object path = sys.attr("path");
	path.attr("insert")(0, config::pylibs_dir);

	py::module_ test = py::module_::import("test1");
	py::function testfunc = test.attr("testfunc");

	py::dict mydict;
	testfunc(mydict, "key", 22);
	testfunc(mydict, "key", 13);
	mydict = py::dict{};
	testfunc(mydict, "key", 8);
}

} //rand_tests namespace












