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
// implements an async token process
// - input token generator creates batches of tokens (1 or more tokens)
// - there are token processing units, the number is equal to the batch size
// - each unit runs a token processing algorithm in its own thread
// - tokens are passed into the units, and results are polled from the processing algorithm
// - input token consumer eats the results
// - when no more tokens will be added, the units are shut down and remaining results collected
//
// - when the async token process has processed all tokens, it gets a final result from the token consumer
///
template <typename TokenProcessorAlgoT, typename FinalResultT>
class AsyncTokenProcess final
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
	using PPackSetT = std::vector<TokenProcessorPack<TokenProcessorAlgoT>>;
	using TokenGenT = std::shared_ptr<TokenBatchGenerator<TokenT>>;
	using TokenConsumeT = std::shared_ptr<TokenBatchConsumer<ResultT>>;

//constructors
	/// default constructor: disabled
	AsyncTokenProcess() = delete;

	/// normal constructor
	AsyncTokenProcess(const int worker_thread_limit,
			const int token_storage_limit,
			const int result_storage_limit,
			TokenGenT token_generator,
			TokenConsumeT token_consumer) : 
		m_worker_thread_limit{worker_thread_limit},
		m_token_storage_limit{token_storage_limit},
		m_result_storage_limit{result_storage_limit},
		m_token_generator{token_generator},
		m_token_consumer{token_consumer},
		m_batch_size{m_token_generator ? m_token_generator->GenBatchSize() : 0}
	{
		static_assert(std::is_base_of<TokenProcessorBase<TokenProcessorAlgoT>, TokenProcessor<TokenProcessorAlgoT>>::value,
			"Token processor implementation does not derive from the TokenProcessorBase!");

		assert(m_token_generator);
		assert(m_token_consumer);
		assert(m_batch_size > 0);
		assert(m_batch_size <= m_worker_thread_limit);
		assert(m_batch_size == m_token_consumer->ConsumeBatchSize());
	}

	/// copy constructor: disabled
	AsyncTokenProcess(const AsyncTokenProcess&) = delete;

//destructor: none

//overloaded operators
	/// asignment operator: disabled
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) = delete;
	AsyncTokenProcess& operator=(const AsyncTokenProcess&) const = delete;

//member functions
	/// run the async token process
	/// have to use std::async for running the AsyncTokenProcess in its own thread
	/// WARNING: if the packs contain shared resources, then running in a new thread may be UB
	std::unique_ptr<FinalResultT> Run(PPackSetT processing_packs)
	{
		if (m_process_state != ProcessState::AVAILABLE)
		{
			assert(false && "async token process can only be run from one thread at a time!");

			return std::unique_ptr<FinalResultT>{};
		}

		m_process_state = ProcessState::RUNNING;

		// make sure processor packs are the right size
		assert(processing_packs.size() == m_batch_size);

		// spawn set of processing units (creates threads)
		std::vector<TokenProcessingUnit<TokenProcessorAlgoT>> processing_units{};
		processing_units.reserve(m_batch_size);

		for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
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
		token_set_shuttle.reserve(m_batch_size);

		// get token set or leave if no more will be created
		while (m_token_generator->GetTokenSet(token_set_shuttle))
		{
			// pass token set to processing units
			// it spins through 'try' functions to avoid deadlocks between token and result queues
			std::size_t remaining_tokens{m_batch_size};
			while (remaining_tokens > 0)
			{
				// recount the number of uninserted tokens each round
				remaining_tokens = 0;

				// iterate through token set to empty it
				for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
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
						m_token_consumer->ConsumeResult(std::move(result_shuttle), unit_index);
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
		for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
		{
			// wait for this unit to stop
			processing_units[unit_index].WaitUntilUnitStops();

			// clear remaining results
			while (processing_units[unit_index].ExtractFinalResults(result_shuttle))
			{
				// sanity check: if a result is obtained, it should exist
				assert(result_shuttle);

				m_token_consumer->ConsumeResult(std::move(result_shuttle), unit_index);
			}
		}

		// get final result (before resetting generator for safety/proper order of events)
		auto final_result{m_token_consumer->GetFinalResult()};

		// reset the token generator
		m_token_generator->Reset();

		// return final result
		m_process_state = ProcessState::AVAILABLE;
		return final_result;
	}

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
	/// batch size (number of tokens per batch)
	const std::size_t m_batch_size{};

	/// token set generator
	TokenGenT m_token_generator{};
	/// token consumer
	TokenConsumeT m_token_consumer{};
};


#endif //header guard




/*
concept for chaining two async token processes together:

// helper to override Process interface
// https://stackoverflow.com/questions/2004820/inherit-interfaces-which-share-a-method-name
class ProcessHelper1 : public Process
{
	ProcessHelper1(...) : Process(...)
	{}

	virtual GetTokenSet(...) override
	{
		GetTokenSet1(...);
	}

	...
}

// same for ProcessHelper2

// it launches two threads to manage the two Process subobjects Run() methods
// which call down to mutex-protected methods in the main object
class Chain : public ProcessHelper1, public ProcessHelper2
{

	

	std::mutex m_mutex;	
}

GetTokenSet1() --(new tokens to Process1)--> ConsumeResult1() --(tokens to mutex-protected queue)-->
GetTokenSet2() --(tokens from queue to Process2)--> ConsumeResult2()



*/















