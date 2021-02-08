// random tests

//local headers
#include "assign_bubbles_algo.h"
#include "cv_vid_bg_helpers.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "ndarray_converter.h"
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
void test_bubblehighlighting(cv::Mat &background_frame, const CommandLinePack &cl_pack, bool add_test_bubbletracking)
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

	if (add_test_bubbletracking)
	{
		test_bubbletracking(*frame);
	}
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

	// pass dictionary to function that mutates it to see if mutations are saved
	py::dict mydict;
	testfunc(mydict, "key", 22);
	testfunc(mydict, "key", 13);

	// reset dictionary
	mydict = py::dict{};
	testfunc(mydict, "key2", 8);

	// try to put the dictionary in a shared pointer
	std::unique_ptr<py::dict> temp_ptr{std::make_unique<py::dict>(std::move(mydict))};
	testfunc(*temp_ptr, "key3", 99);
	py::dict newdict{std::move(*(temp_ptr.release()))};
	testfunc(newdict, "key4", 1000);
	bool move_succeeded{false};
	try { testfunc(mydict, "key5", 123456); } catch (...) { move_succeeded = true; }
	assert(move_succeeded);

	// can recreate the dictionary after move
	mydict = py::dict{};
	testfunc(mydict, "key6", 654321);

	// test custom converter
	// set up the Numpy array <-> cv::Mat converter
	// courtesy of https://github.com/edmBernard/pybind11_opencv_numpy
	NDArrayConverter::init_numpy();

	cv::Mat testmat{5,10, CV_8UC1};
	testfunc(mydict, "mat", testmat);
}

/// note: assumes test_frame contains highlighted bubbles
/// this test is just to see if the AssignBubblesAlgo works with one input frame
void test_bubbletracking(cv::Mat &test_frame)
{
	// create Python interpreter
	py::scoped_interpreter guard{};

	// add location of bubbletracking python libs to path so they can be found
	py::module_ sys = py::module_::import("sys");
	py::object path = sys.attr("path");
	std::string lib_dir{config::bubbletracking_dir};
	lib_dir += "/src/";
	path.attr("insert")(0, lib_dir.c_str());

	// set parameters
	TokenProcessorPack<AssignBubblesAlgo> assign_bubbles_pack{
		"cvimproc.improc",
		py::make_tuple(1.0f, 0), 	// +x direction (?)
		3,		// not useful here...
		4,
		5,
		200,
		40
	};

	// create new scope so the GIL can be released
	std::unique_ptr<py::dict> bubbles_archive{nullptr};
	{
		// release GIL (will be acquired by AssignBubblesAlgo)
		py::gil_scoped_release nogil;

		// create algo object for processing frames (highlighting bubbles)
		AssignBubblesAlgo assign_bubbles{assign_bubbles_pack};

		// convert format
		std::vector<cv::Mat> vec_temp;
		vec_temp.emplace_back(test_frame.clone());
		std::unique_ptr<std::vector<cv::Mat>> highlighted_bubbles{std::make_unique<std::vector<cv::Mat>>(std::move(vec_temp))};

		// insert to algo (processes the frame)
		assign_bubbles.Insert(std::move(highlighted_bubbles));

		// no more frames to insert (tells the algo to finalize a result)
		assign_bubbles.NotifyNoMoreTokens();

		bubbles_archive = assign_bubbles.TryGetResult();
	}

	// print result
	py::print(*bubbles_archive);
}

} //rand_tests namespace












