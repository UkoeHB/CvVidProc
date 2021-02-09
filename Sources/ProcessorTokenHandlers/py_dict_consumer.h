// implementation of TokenBatchConsumer for acquiring py::dict objects

#ifndef PY_DICT_CONSUMER_756789_H
#define PY_DICT_CONSUMER_756789_H

//local headers
#include "token_batch_consumer.h"
#include <pybind11/pybind11.h>

//third party headers

//standard headers
#include <cassert>
#include <list>
#include <memory>


/// does not care if inputs are batched, just stores them in a list as they appear
/// the goal of storing them in unique_ptrs is to avoid interacting with the python GIL
class PyDictConsumer final : public TokenBatchConsumer<py::dict, std::list<std::unique_ptr<py::dict>>>
{
public:
//constructors
	/// default constructor: disabled
	PyDictConsumer() = delete;

	/// normal constructor
	PyDictConsumer(const int batch_size,
			const bool collect_timings) : 
		TokenBatchConsumer{batch_size, collect_timings}
	{}

	/// copy constructor: disabled
	PyDictConsumer(const PyDictConsumer&) = delete;

//destructor: not needed

//overloaded operators
	/// asignment operator: disabled
	PyDictConsumer& operator=(const PyDictConsumer&) = delete;
	PyDictConsumer& operator=(const PyDictConsumer&) const = delete;

protected:
//member functions
	/// consume a py::dict
	virtual void ConsumeTokenImpl(std::unique_ptr<token_type> token, const std::size_t index_in_batch) override
	{
		assert(index_in_batch < GetBatchSize());

		if (!m_results)
			m_results = std::make_unique<final_result_type>(final_result_type{});

		m_results->emplace_back(std::move(token));
	}

	/// get final result (list of py::dicts in the order they were obtained)
	virtual std::unique_ptr<final_result_type> GetFinalResult() override
	{
		return std::move(m_results);
	}

private:
//member variables
	/// store image fragments until they are ready to be used
	std::unique_ptr<final_result_type> m_results{nullptr};
};


#endif //header guard
















