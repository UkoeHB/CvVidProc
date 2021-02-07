// python bindings for opencv vid processing

//local headers
#include "cv_vid_bg_helpers.h"
#include "main.h"
#include "ndarray_converter.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>

//standard headers
#include <cstdint>
#include <memory>

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

	/// get number of worker threads to use from max threads available (subtract one for the main thread)
	/// note: min threads returned is 1
	mod.def("WorkerThreadsFromMax", &WorkerThreadsFromMax, "get number of worker threads to use (subtract one from max for the main thread); min return value is '1'",
		py::arg("max"));

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
				const int,
				const bool>(),
				py::arg("vid_path"),
				py::arg("bg_algo"),
				py::arg("batch_size"),
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
				py::arg("result_storage_limit") = 200,
				py::arg("print_timing_report") = false)
		.def_readonly("vid_path", &VidBgPack::vid_path)
		.def_readonly("bg_algo", &VidBgPack::bg_algo)
		.def_readonly("batch_size", &VidBgPack::batch_size)
		.def_readonly("frame_limit", &VidBgPack::frame_limit)
		.def_readonly("grayscale", &VidBgPack::grayscale)
		.def_readonly("vid_is_grayscale", &VidBgPack::vid_is_grayscale)
		.def_readonly("crop_x", &VidBgPack::crop_x)
		.def_readonly("crop_y", &VidBgPack::crop_y)
		.def_readonly("crop_width", &VidBgPack::crop_width)
		.def_readonly("crop_height", &VidBgPack::crop_height)
		.def_readonly("horizontal_buffer_pixels", &VidBgPack::horizontal_buffer_pixels)
		.def_readonly("vertical_buffer_pixels", &VidBgPack::vertical_buffer_pixels)
		.def_readonly("token_storage_limit", &VidBgPack::token_storage_limit)
		.def_readonly("result_storage_limit", &VidBgPack::result_storage_limit)
		.def_readonly("print_timing_report", &VidBgPack::print_timing_report);

	/// funct GetVideoBackground()
	mod.def("GetVideoBackground", &GetVideoBackground, "Get the background of an OpenCV video.",
		py::arg("pack"));
}













