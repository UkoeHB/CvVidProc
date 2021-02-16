// python bindings for opencv vid processing

//local headers
#include "assign_bubbles_algo.h"
#include "cv_vid_bubbletrack_helpers.h"
#include "cv_vid_bg_helpers.h"
#include "highlight_bubbles_algo.h"
#include "main.h"
#include "ndarray_converter.h"
#include "token_processor_algo.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>

//standard headers
#include <cstdint>
#include <memory>
#include <string>


namespace py = pybind11;

/// create module
/// NOTE: must update __init__.py file when new symbols are added
PYBIND11_MODULE(_core, mod)
{
	/// set up the Numpy array <-> cv::Mat converter
	/// courtesy of https://github.com/edmBernard/pybind11_opencv_numpy
	NDArrayConverter::init_numpy();

	/// info
	mod.doc() = "C++ bindings for processing an opencv video";

	/// struct VidBgPack binding
	py::class_<VidBgPack>(mod, "VidBgPack")
		.def(py::init<const std::string&,
				const std::string&,
				const int,
				const long long,
				const bool,
				const bool,
				const int,
				const int,
				const int,
				const int,
				const int,
				const int,
				const int,
				const bool>(),
				py::arg("vid_path"),
				py::arg("bg_algo") = "hist",
				py::arg("max_threads") = -1,
				py::arg("frame_limit") = -1,
				py::arg("grayscale") = false,
				py::arg("vid_is_grayscale") = false,
				py::arg("crop_x") = 0,
				py::arg("crop_y") = 0,
				py::arg("crop_width") = 0,
				py::arg("crop_height") = 0,
				py::arg("horizontal_buffer_pixels") = 0,
				py::arg("vertical_buffer_pixels") = 0,
				py::arg("token_storage_limit") = 200,
				py::arg("print_timing_report") = false);

	/// funct GetVideoBackground()
	mod.def("GetVideoBackground", &GetVideoBackground, "Get the background of an OpenCV video.",
		// no need to hold the GIL in long-running C++ code
		py::call_guard<py::gil_scoped_release>(),
		py::arg("pack"));	//VidBgPack

	/// struct TokenProcessorPack<HighlightBubblesAlgo>
	py::class_<TokenProcessorPack<HighlightBubblesAlgo>>(mod, "HighlightBubblesPack")
		.def(py::init<cv::Mat,
				cv::Mat,
				const int,
				const int,
				const int,
				const int,
				const int,
				const int>(),
				py::arg("background"),
				py::arg("struct_element"),
				py::arg("threshold"),
				py::arg("threshold_lo"),
				py::arg("threshold_hi"),
				py::arg("min_size_hyst"),
				py::arg("min_size_threshold"),
				py::arg("width_border"));

	/// struct TokenProcessorPack<AssignBubblesAlgo>
	py::class_<TokenProcessorPack<AssignBubblesAlgo>>(mod, "AssignBubblesPack")
		.def(py::init<py::function,
				py::dict>(),
				"Expected signature of input func: \
				f(frame_bw, f, bubbles_prev, bubbles_archive, ID_curr, args) \
				note: 'args' should be a python dictionary",
				py::args("function"),
				py::args("args"));

	/// struct VidBubbleTrackPack
	py::class_<VidBubbleTrackPack>(mod, "VidBubbleTrackPack")
		.def(py::init<const std::string,
				TokenProcessorPack<HighlightBubblesAlgo>,
				TokenProcessorPack<AssignBubblesAlgo>,
				const int,
				const long long,
				const bool,
				const bool,
				const int,
				const int,
				const int,
				const int,
				const int,
				const bool>(),
				py::arg("vid_path"),
				py::arg("highlightbubbles_pack"),
				py::arg("assignbubbles_pack"),
				py::arg("max_threads") = -1,
				py::arg("frame_limit") = -1,
				py::arg("grayscale") = false,
				py::arg("vid_is_grayscale") = false,
				py::arg("crop_x") = 0,
				py::arg("crop_y") = 0,
				py::arg("crop_width") = 0,
				py::arg("crop_height") = 0,
				py::arg("token_storage_limit") = -1,
				py::arg("print_timing_report") = false);

	/// funct TrackBubbles()
	mod.def("TrackBubbles", &TrackBubbles, "Track bubbles in an OpenCV video.",
		py::arg("pack"));	//VidBubbleTrackPack
}













