// python bindings for opencv vid processing

//local headers
#include "cv_vid_bg_helpers.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)
#include <pybind11/pybind11.h>

//standard headers
#include <cstdint>
#include <memory>

namespace py = pybind11;


/// create module
PYBIND11_MODULE(cvvidproc, mod)
{
	/// info
	mod.doc() = "C++ bindings for processing an opencv video";

	/// struct VidBgPack binding
	py::class_<VidBgPack>(mod, "VidBgPack")
		.def(py::init<const std::string&,
					const int,
					const long long,
					const long long,
					const bool,
					const int,
					const int,
					const int,
					const int>())
		.def_readonly("bg_algo", &VidBgPack::bg_algo)
		.def_readonly("batch_size", &VidBgPack::batch_size)
		.def_readonly("total_frames", &VidBgPack::total_frames)
		.def_readonly("frame_limit", &VidBgPack::frame_limit)
		.def_readonly("grayscale", &VidBgPack::grayscale)
		.def_readonly("horizontal_buffer_pixels", &VidBgPack::horizontal_buffer_pixels)
		.def_readonly("vertical_buffer_pixels", &VidBgPack::vertical_buffer_pixels)
		.def_readonly("token_storage_limit", &VidBgPack::token_storage_limit)
		.def_readonly("result_storage_limit", &VidBgPack::result_storage_limit);

	/// funct GetVideoBackground()
	mod.def("GetVideoBackground", &GetVideoBackground, "Get the background of an OpenCV video.",
		py::arg("vid"),
		py::arg("pack"));
}













