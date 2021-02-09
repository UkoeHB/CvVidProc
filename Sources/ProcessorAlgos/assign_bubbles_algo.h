// identifies bubbles in a series of images, and tracks them

#ifndef ASSIGN_BUBBLES_ALGO_67654545_H
#define ASSIGN_BUBBLES_ALGO_67654545_H

//local headers
#include "ndarray_converter.h"
#include "project_dir_config.h"
#include "token_processor_algo_base.h"

//third party headers
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>

//standard headers
#include <memory>
#include <string>
#include <type_traits>
#include <vector>


namespace py = pybind11;

/// processor algorithm type declaration
class AssignBubblesAlgo;

/// specialization of variable pack
template <>
struct TokenProcessorPack<AssignBubblesAlgo> final
{
	std::string bubbletracking_module;
	std::string bubbletracking_function;
	py::tuple flow_dir;
	const int fps;
	const int pix_per_um;
	const int width_border;
	const double v_max;
	const int min_size_reg;
};

////
// implementation for algorithm: assign bubbles
// obtains a vector of ordered cv::Mats and passes them to a Python method (this is a glorified wrapper)
// keeps track of all the bubbles that have been identified, and the most recent 'active bubbles'
// 	so bubbles that appear in more than one frame can be tracked
// note: expects the bubbletracking module to be available in sys.path, which may need to be pointed to by the caller
//  #include "project_dir_config.h"
//
// 	py::module_ sys = py::module_::import("sys");
//  py::object path = sys.attr("path");
//  path.attr("insert")(0, config::bubbletracking_dir);
///
class AssignBubblesAlgo final : public TokenProcessorAlgoBase<AssignBubblesAlgo, std::vector<cv::Mat>, py::dict>
{
public:
//constructors
	/// default constructor: disabled
	AssignBubblesAlgo() = delete;

	/// normal constructor
	AssignBubblesAlgo(TokenProcessorPack<AssignBubblesAlgo> processor_pack) :
		TokenProcessorAlgoBase{std::move(processor_pack)}
	{
		// Python GIL acquire (for interacting with python; blocks if another thread has the GIL)
		py::gil_scoped_acquire gil;

		// prepare the wrapped function
		py::module_ algo_module = py::module_::import(m_pack.bubbletracking_module.c_str());
		m_algo_func = algo_module.attr(bubbletracking_function);
	}

	/// copy constructor: disabled
	AssignBubblesAlgo(const AssignBubblesAlgo&) = delete;

//destructor
	~AssignBubblesAlgo()
	{
		// it is best to always call NotifyNoMoreTokens() before destroying the algo, which will clean up the python resources,
		// so you don't have to acquire the GIL during destruction (might cause problems)
		if (m_bubbles_active || m_bubbles_archive)
		{
			// Python GIL acquire (for interacting with python; blocks if another thread has the GIL)
			py::gil_scoped_acquire gil;

			// destroy the python dictionaries within the GIL
			m_bubbles_active = nullptr;
			m_bubbles_archive = nullptr;
		}
	}

//overloaded operators
	/// copy assignment operators: disabled
	AssignBubblesAlgo& operator=(const AssignBubblesAlgo&) = delete;
	AssignBubblesAlgo& operator=(const AssignBubblesAlgo&) const = delete;

//member functions
	/// insert an element to be processed
	/// note: overrides result if there already is one
	virtual void Insert(std::unique_ptr<std::vector<cv::Mat>> in_mats) override
	{
		// leave if images not available
		if (!in_mats || !in_mats->size())
			return;

		// Python GIL acquire (for running python functions; blocks if another thread has the GIL)
		py::gil_scoped_acquire gil;

		// set up the Numpy array <-> cv::Mat converter
		// courtesy of https://github.com/edmBernard/pybind11_opencv_numpy
		NDArrayConverter::init_numpy();

		// make sure the dictionaries are available
		if (!m_bubbles_active)
			m_bubbles_active = std::make_unique<py::dict>();

		if (!m_bubbles_archive)
			m_bubbles_archive = std::make_unique<py::dict>();

		// process each input image
		for (auto image : *in_mats)
		{
			// check each Mat in the vector
			if (!image.data || image.empty())
				continue;

			// process the Mat
			// - python call
			using namespace pybind11::literals;		// for '_a'
			m_current_id = m_algo_func("frame_bw"_a = image,
				"f"_a = m_num_processed,
				"bubbles_prev"_a = *m_bubbles_active,
				"bubbles_archive"_a = *m_bubbles_archive,
				"ID_curr"_a = m_current_id,
				"flow_dir"_a = m_pack.flow_dir,
				"fps"_a = m_pack.fps,
				"pix_per_um"_a = m_pack.pix_per_um,
				"width_border"_a = m_pack.width_border,
				"row_lo"_a = 0,
				"row_hi"_a = image.rows,
				"v_max"_a = m_pack.v_max,
				"min_size_reg"_a = m_pack.min_size_reg
			).cast<int>();

			m_num_processed++;
		}
	}

	/// get the processing result
	virtual std::unique_ptr<py::dict> TryGetResult() override
	{
		// get result if there is one
		if (m_result)
			return std::move(m_result);
		else
			return nullptr;
	}

	/// get notified there are no more elements
	virtual void NotifyNoMoreTokens() override
	{
		// Python GIL acquire (for interacting with python; blocks if another thread has the GIL)
		py::gil_scoped_acquire gil;

		// move the bubbles archive into the result variable so it can be extracted
		// - no more frames will be processed so we are done
		m_result = std::move(m_bubbles_archive);

		// reset variables
		m_num_processed = 0;
		m_current_id = 0;
		m_bubbles_active = nullptr;
		m_bubbles_archive = nullptr;
	}

private:
//member variables
	/// keep track of wrapped python function we want to run
	py::function m_algo_func;

	/// number of frames processed
	int m_num_processed{0};
	/// current ID
	int m_current_id{0};
	/// still-alive bubbles
	std::unique_ptr<py::dict> m_bubbles_active;
	/// all bubbles encountered
	std::unique_ptr<py::dict> m_bubbles_archive;

	/// store result in anticipation of future requests
	std::unique_ptr<py::dict> m_result{};
};


#endif //header guard



















