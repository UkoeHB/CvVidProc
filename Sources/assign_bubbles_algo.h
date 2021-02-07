// identifies bubbles in a series of images, and tracks them

#ifndef ASSIGN_BUBBLES_ALGO_67654545_H
#define ASSIGN_BUBBLES_ALGO_67654545_H

//local headers
#include "token_processor_algo_base.h"

//third party headers
#include <opencv2/opencv.hpp>
#include <pybind11/pybind11.h>

//standard headers
#include <memory>
#include <type_traits>
#include <vector>


/// processor algorithm type declaration
class AssignBubblesAlgo;

/// specialization of variable pack
template <>
struct TokenProcessorPack<AssignBubblesAlgo> final
{};

////
// implementation for algorithm: assign bubbles
// obtains a vector of ordered cv::Mats and passes them to a Python method (this is a glorified wrapper)
// keeps track of all the bubbles that have been identified, and the most recent 'active bubbles'
// 	so bubbles that appear in more than one frame can be tracked
///
class AssignBubblesAlgo final : public TokenProcessorAlgoBase<AssignBubblesAlgo, std::vector<cv::Mat>, py::dict>
{
public:
//constructors
	/// default constructor: disabled
	AssignBubblesAlgo() = delete;

	/// normal constructor
	AssignBubblesAlgo(TokenProcessorPack<AssignBubblesAlgo<T>> processor_pack) :
		TokenProcessorAlgoBase<AssignBubblesAlgo, std::vector<cv::Mat>, py::dict>{std::move(processor_pack)}
	{
		// Python GIL acquire (for interacting with python; blocks if another thread has the GIL)
		//TODO: is this necessary here?
		py::gil_scoped_acquire gil;

		// prepare the wrapped function
		py::object algo_module = py::module_::import("...");
		m_algo_func = algo_module.attr("...");
	}

	/// copy constructor: disabled
	AssignBubblesAlgo(const AssignBubblesAlgo&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	AssignBubblesAlgo& operator=(const AssignBubblesAlgo&) = delete;
	AssignBubblesAlgo& operator=(const AssignBubblesAlgo&) const = delete;

//member functions
	/// insert an element to be processed
	/// note: overrides result if there already is one
	virtual void Insert(std::unique_ptr<std::vector<cv::Mat>> in_mats) override
	{
		// leave if image not available
		if (!in_mats || !in_mat->size() || in_mat->empty())
			return;

		// Python GIL acquire (for running python functions; blocks if another thread has the GIL)
		py::gil_scoped_acquire gil;

		for (auto image : *in_mats)
		{
			// check each Mat in the vector
			if (!image.data || image.empty())
				continue;

			// process the Mat
			// - python call
			m_algo_func("...");
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
		// move the bubbles archive into the result variable so it can be extracted
		// - no more frames will be processed so we are done
	}

private:
//member variables
	/// keep track of wrapped python function we want to run
	py::object m_algo_func;

	/// 

	/// store result in anticipation of future requests
	std::unique_ptr<py::dict> m_result{};
};


#endif //header guard



















