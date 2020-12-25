// async batchable token processing process

#ifndef ASYNC_TOKEN_PROCESS_4567898765_H
#define ASYNC_TOKEN_PROCESS_4567898765_H

//local headers
#include "token_processor.h"
#include "token_processing_unit.h"

//third party headers

//standard headers
#include <atomic>
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
		AVAILABLE,
		RUNNING
	};

public:
	using TokenT = typename TokenProcessorAlgoT::token_type;
	using ResultT = typename TokenProcessorAlgoT::result_type;

//constructors
	/// default constructor: disabled
	AsyncTokenProcess() = delete;

	/// normal constructor
	AsyncTokenProcess(const int worker_thread_limit, const int token_storage_limit, const int result_storage_limit) : 
		m_worker_thread_limit{worker_thread_limit},
		m_token_storage_limit{token_storage_limit},
		m_result_storage_limit{result_storage_limit}
	{
		static_assert(std::is_base_of<TokenProcessorBase<TokenProcessorAlgoT>, TokenProcessor<TokenProcessorAlgoT>>::value,
			"Token processor implementation does not derive from the TokenProcessorBase!");
	}

	/// copy constructor: disabled
	AsyncTokenProcess(const AsyncTokenProcess&) = delete;

	/// destructor
	virtual ~AsyncTokenProcess() = default;

//overloaded operators
	/// asignment operator: disabled
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) = delete;
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) const = delete;

	/// parens operator, for running the AsyncTokenProcess in its own thread
	/// WARNING: if the master process owns shared resources with the caller, then running in a new thread is UB
	std::unique_ptr<FinalResultT> operator()() { return Run(); }

//member functions
	/// run the async token process
	std::unique_ptr<FinalResultT> Run()
	{
		if (m_process_state != ProcessState::AVAILABLE)
		{
			assert(false && "async token process can only be run from one thread at a time!");

			return std::unique_ptr<FinalResultT>{};
		}

		m_process_state = ProcessState::RUNNING;

		// get processor packs for processing units
		auto processing_packs{GetProcessingPacks()};

		// get batch size
		std::size_t batch_size{static_cast<std::size_t>(GetBatchSize())};

		assert(batch_size > 0 && batch_size <= m_worker_thread_limit);
		assert(batch_size == processing_packs.size());

		// spawn set of processing units (creates threads)
		std::vector<TokenProcessingUnit<TokenProcessorAlgoT>> processing_units{};
		processing_units.reserve(batch_size);

		for (std::size_t unit_index{0}; unit_index < batch_size; unit_index++)
		{
			// according to https://stackoverflow.com/questions/5410035/when-does-a-stdvector-reallocate-its-memory-array
			// this will not reallocate the vector unless the .reserve() amount is exceeded, so it should be thread safe
			processing_units.emplace_back(m_token_storage_limit, m_result_storage_limit);

			// start the unit's thread
			processing_units[unit_index].Start(std::move(processing_packs[unit_index]));
		}

		// consume tokens until no more are generated
		std::vector<std::unique_ptr<TokenT>> token_set_shuttle{};
		std::unique_ptr<ResultT> result_shuttle{};
		token_set_shuttle.reserve(batch_size);

		// get token set or leave if no more will be created
		while (GetTokenSet(token_set_shuttle))
		{
			// pass token set to processing units
			// it spins through 'try' functions to avoid deadlocks between token and result queues
			std::size_t remaining_tokens{batch_size};
			while (remaining_tokens > 0)
			{
				// recount the number of uninserted tokens each round
				remaining_tokens = 0;

				// iterate through token set to empty it
				for (std::size_t unit_index{0}; unit_index < batch_size; unit_index++)
				{
					// try to insert the token to its processing unit (if it needs to be inserted)
					if (token_set_shuttle[unit_index])
					{
						if (processing_units[unit_index].TryInsert(token_set_shuttle[unit_index]))
						{
							// sanity check - successful insertions should remove the token
							assert(!token_set_shuttle[unit_index]);
						}
						else
							remaining_tokens++;
					}

					// try to get a result from the processing unit
					if (processing_units[unit_index].TryGetResult(result_shuttle))
					{
						// sanity check: if a result is obtained, it should exist
						assert(result_shuttle);

						// consume the result
						ConsumeResult(std::move(result_shuttle), unit_index);
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
				// sanity check: if a result is obtained, it should exist
				assert(result_shuttle);

				ConsumeResult(std::move(result_shuttle), unit_index);
			}
		}

		// get final result (before resetting generator for safety)
		auto final_result{GetFinalResult()};

		// reset the token generator
		Reset();

		// return final result
		m_process_state = ProcessState::AVAILABLE;
		return final_result;
	}

protected:
	/// get batch size (number of tokens in each batch)
	virtual int GetBatchSize() = 0;

	/// get processing packs for processing units (num packs = intended batch size)
	virtual std::vector<TokenProcessorPack<TokenProcessorAlgoT>> GetProcessingPacks() = 0;

	/// get token set from generator; should return false when no more token sets to get
	virtual bool GetTokenSet(std::vector<std::unique_ptr<TokenT>> &return_token_set) = 0;

	/// consume an intermediate result from one of the token processing units
	virtual void ConsumeResult(std::unique_ptr<ResultT> intermediate_result, const std::size_t index_in_batch) = 0;

	/// get final result
	virtual std::unique_ptr<FinalResultT> GetFinalResult() = 0;

	/// reset the token generator
	virtual void Reset() = 0;


private:
//member variables
	/// max number of worker threads allowed
	const int m_worker_thread_limit{};
	/// keep track of the process state
	std::atomic<ProcessState> m_process_state{ProcessState::AVAILABLE};

	/// max number of tokens that can be stored in each processing unit
	const int m_token_storage_limit{};
	/// max number of results that can be stored in each processing unit
	const int m_result_storage_limit{};

};


#endif //header guard




















