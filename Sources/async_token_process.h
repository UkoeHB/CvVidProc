// async batchable token processing process

#ifndef ASYNC_TOKEN_PROCESS_4567898765_H
#define ASYNC_TOKEN_PROCESS_4567898765_H

//local headers
#include "token_processing_unit.h"

//third party headers

//standard headers
#include <memory>
#include <type_traits>


////
// abstract class for an async token process
// - [IMPLEMENTATION DEFINED]: token generator produces token batches (1 or more tokens)
// - there are token processing units, the number is equal to the batch size
// - each unit runs a token processing algorithm in its own thread
// - tokens are passed into the units, and results are polled from the processing algorithm
// - [IMPLEMENTATION DEFINED]: results are handled continuously
// - when no more tokens will be added, the units are shut down and remaining results collected
//
// - this class has an overloaded operator() so it can be run from a thread of its own
// - [IMPLEMENTATION DEFINED]: when the async token process has consumed and processed all tokens, it makes a final result available
///
template <typename TokenProcessorAlgoT, typename FinalResultT>
class AsyncTokenProcess
{
//member types
	enum class ProcessState
	{
		UNUSED,
		RUNNING,
		COMPLETE
	};

public:
	using TokenT = TokenProcessorAlgoT::token_type;
	using ResultT = TokenProcessorAlgoT::result_type;

//constructors
	/// default constructor: disabled
	AsyncTokenProcess() = delete;

	/// normal constructor
	AsyncTokenProcess(const int worker_threads, const int token_storage_limit, const int result_storage_limit) : 
		m_worker_threads{worker_threads},
		m_token_storage_limit{token_storage_limit},
		m_result_storage_limit{result_storage_limit}
	{}

	/// copy constructor: disabled
	AsyncTokenProcess(const AsyncTokenProcess&) = delete;

	/// destructor
	virtual ~AsyncTokenProcess() = default;

//overloaded operators
	/// asignment operator: disabled
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) = delete;
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) const = delete;

	/// parens operator, for running the asynctokenprocess in its own thread
	std::unique_ptr<FinalResultT> operator()() { return std::move(Run()); }

//member functions
	/// run the async token process
	std::unique_ptr<FinalResultT> Run()
	{
		assert(m_process_state == ProcessState::UNUSED && "async token process can only be run once!");
		m_process_state = ProcessState::RUNNING;

		// get processor packs for processing units
		auto processing_packs{GetProcessingPacks()};

		// assume token batch size equals number of packs obtained
		std::size_t batch_size{processing_packs.size()};

		assert(batch_size > 0 && batch_size <= m_worker_threads);

		// spawn set of processing units (creates threads)
		std::vector<TokenProcessingUnit<TokenProcessorAlgoT>> processing_units{};
		processing_units.reserve(batch_size);

		for (auto &pack : processing_packs)
		{
			// according to https://stackoverflow.com/questions/5410035/when-does-a-stdvector-reallocate-its-memory-array
			// this will not reallocate the vector unless the .reserve() amount is exceeded, so it should be thread safe
			processing_units.emplace_back(pack, m_token_storage_limit, m_result_storage_limit);
		}

		// consume tokens until no more are generated
		std::vector<std::unique_ptr<TokenT>> token_set_shuttle{};
		std::unique_ptr<ResultT> result_shuttle{};
		token_set_shuttle.reserve(batch_size);

		while (true)
		{
			// get token set or leave
			if (!GetTokenSet(token_set_shuttle))
				break;

			// pass token set to processing units
			// it spins through 'try' functions to avoid deadlocks between token and result queues
			int remaining_tokens{batch_size};
			while (remaining_tokens > 0)
			{
				// iterate through token set to empty it
				for (std::size_t unit_index{0}; unit_index < batch_size; unit_index++)
				{
					// try to insert the token to its processing unit (if it needs to be inserted)
					if (token_set_shuttle[unit_index])
					{
						if (processing_units[unit_index].TryInsert(token_set_shuttle[unit_index]))
							remaining_tokens--;
					}

					// try to get a result from the processing unit
					if (processing_units[unit_index].TryGetResult(result_shuttle))
					{
						// consume the result
						ConsumeResult(result_shuttle, unit_index);
					}
				}
			}
		}

		// shut down all processing units (no more tokens)
		for (auto &unit : processing_units)
		{
			unit.ShutDown();
		}

		// wait for all units to shut down, and clear out their remaining results
		// shut down and wait-for-stop are separate so all units can do shut down procedure in parallel
		for (std::size_t unit_index{0}; unit_index < batch_size; unit_index++)
		{
			// wait for this unit to stop
			processing_units[unit_index].WaitUntilUnitStops();

			// clear remaining results
			while (processing_units[unit_index].ExtractFinalResults(result_shuttle))
			{
				ConsumeResult(result_shuttle, unit_index);
			}
		}

		// return final result
		m_process_state = ProcessState::COMPLETE;
		return std::move(GetFinalResult());
	}

	/// get processing packs for processing units (num packs = intended batch size)
	virtual std::vector<TokenProcessorPack<TokenProcessorAlgoT>> GetProcessingPacks() = 0;

	/// get token set from generator
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSet() = 0;

	/// consume an intermediate result from one of the token processing units
	virtual void ConsumeResult(std::unique_ptr<ResultT> &intermediate_result, const std::size_t index_in_batch) = 0;

	/// get final result
	virtual std::unique_ptr<FinalResultT> GetFinalResult() = 0;


private:
//member variables
	/// number of worker threads allowed
	const int m_worker_threads{};
	/// keep track of the process state
	ProcessState m_process_state{ProcessState::UNUSED};

	/// max number of tokens that can be stored in each processing unit
	const int m_token_storage_limit{}
	/// max number of results that can be stored in each processing unit
	const int m_result_storage_limit{}

};
/*

- spawn set of processing units

while(tokens left)

	- get token set

	while (token set not emptied)
	{
		- iterate through token set
			- try to insert token
			- try to get result
	}

- shut down units

- wait until units are stopped

- clear out remaining results

*/

#endif //header guard




















