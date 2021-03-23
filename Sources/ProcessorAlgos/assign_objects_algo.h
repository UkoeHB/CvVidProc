// identifies objects in a series of images, and tracks them

#ifndef ASSIGN_OBJECTS_ALGO_67654545_H
#define ASSIGN_OBJECTS_ALGO_67654545_H

//local headers
#include "ndarray_converter.h"
#include "project_config.h"
#include "token_processor_algo.h"

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
class AssignObjectsAlgo;

/// specialization of variable pack
template <>
struct __attribute__ ((visibility("hidden"))) TokenProcessorPack<AssignObjectsAlgo> final
{
	////
	// expected function signature:
	// next_ID = func(bw_frame, frames_processed, objects_prev, objects_archive, next_ID, kwargs)
	// bw_frame: black/white frame
	// frames_processed: number of frames processed so far
	// objects_prev: python Dictionary of active objects
	// objects_archive: python Dictionary of all objects encountered
	// next_ID: ID of next object to be assigned (updated via return value of func())
	// kwargs: python Dict of user-defined parameters (can be anything, even malleable types)
	///
	py::function object_tracking_function{};
	/// miscellaneous user-defined args that will be passed to the object tracking function
	py::dict kwargs{};
};

////
// wrapper for python implementation of object tracking
//
// obtains a vector of ordered cv::Mats and passes them sequentially to the python function
// keeps a record of all objects encountered, and the most recent 'active objects'
// 	so objects that appear in more than one frame can be tracked
//
// WARNING: if the GIL is not held when this object is destroyed, there will be a SEGFAULT (TODO: fixme)
///
class __attribute__ ((visibility("hidden"))) AssignObjectsAlgo final : public TokenProcessorAlgo<AssignObjectsAlgo, std::vector<cv::Mat>, py::dict>
{
public:
//constructors
	/// default constructor: disabled
	AssignObjectsAlgo() = delete;

	/// normal constructor
	AssignObjectsAlgo(TokenProcessorPack<AssignObjectsAlgo> processor_pack) :
		TokenProcessorAlgo{std::move(processor_pack)}
	{}

	/// copy constructor: disabled
	AssignObjectsAlgo(const AssignObjectsAlgo&) = delete;

//destructor
	~AssignObjectsAlgo()
	{
		// Python GIL acquire (for interacting with python; blocks if another thread has the GIL)
		//TODO: not clear if acquiring gil is adequate here - might be necessary for dtor caller to have it
		py::gil_scoped_acquire gil;

		// destroy the python dictionaries within the GIL
		m_objects_active = nullptr;
		m_objects_archive = nullptr;
		m_result = nullptr;
		{
			// call destructors on python objects
			TokenProcessorPack<AssignObjectsAlgo> temp{std::move(m_pack)};
		}
	}

//overloaded operators
	/// copy assignment operators: disabled
	AssignObjectsAlgo& operator=(const AssignObjectsAlgo&) = delete;
	AssignObjectsAlgo& operator=(const AssignObjectsAlgo&) const = delete;

//member functions
	/// inserts frames to algo
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
		if (!m_objects_active)
			m_objects_active = std::make_unique<py::dict>();

		if (!m_objects_archive)
			m_objects_archive = std::make_unique<py::dict>();

		// process each input image
		for (auto image : *in_mats)
		{
			// check each Mat in the vector
			if (!image.data || image.empty())
				continue;

			// process the Mat
			// - python call
			using namespace pybind11::literals;		// for '_a'
			m_next_id = m_pack.object_tracking_function("bw_frame"_a = image,
				"frames_processed"_a = m_num_processed,
				"objects_prev"_a = *m_objects_active,
				"objects_archive"_a = *m_objects_archive,
				"next_ID"_a = m_next_id,
				"kwargs"_a = *m_pack.kwargs
			).cast<int>();

			m_num_processed++;
		}
	}

	/// get the processing result
	virtual std::unique_ptr<result_type> TryGetResult() override
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

		// move the object archive into the result variable so it can be extracted
		// - no more frames will be processed so we are done
		m_result = std::move(m_objects_archive);

		// reset variables
		m_num_processed = 0;
		m_next_id = 0;
		m_objects_active = nullptr;
		m_objects_archive = nullptr;
	}

	/// report if there is a result to get
	virtual bool HasResults() override
	{
		return static_cast<bool>(m_result);
	}

private:
//member variables
	/// number of frames processed
	int m_num_processed{0};
	/// current ID
	int m_next_id{0};
	/// still-alive objects
	std::unique_ptr<py::dict> m_objects_active;
	/// all objects encountered
	std::unique_ptr<py::dict> m_objects_archive;

	/// store result in anticipation of future requests
	std::unique_ptr<py::dict> m_result{};
};


#endif //header guard



















