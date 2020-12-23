// processes tokens in a worker thread with the specified algorithm

#ifndef TOKEN_PROCESSING_UNIT_32573592_H
#define TOKEN_PROCESSING_UNIT_32573592_H

//local headers
#include "async_token_queue.h"
#include "token_processor.h"

//third party headers

//standard headers
#include <cassert>
#include <memory>
#include <thread>
#include <type_traits>


////
// expected usage:
// 		- the class specialization TokenProcessor<TokenProcessorAlgoT> must
//			be derived from TokenProcessorBase<TokenProcessorAlgoT>
//		note: c++20 contracts would make this easier...
/// 
template <typename TokenProcessorAlgoT>
class TokenProcessingUnit final
{
public:
//member types
	/// token processor type
	using TokenProcessorT = TokenProcessor<TokenProcessorAlgoT>;
	/// get token type from token processor impl
	using TokenT = typename TokenProcessorT::TokenT;
	/// get result type from token processor impl
	using ResultT = typename TokenProcessorT::ResultT;
	/// token queue type
	using TokenQueueT = AsyncTokenQueue<std::unique_ptr<TokenT>>;
	/// result queue type
	using ResultQueueT = AsyncTokenQueue<std::unique_ptr<ResultT>>;

//constructors
	/// default constructor: default (not meant to be used, just for placeholding)
	TokenProcessingUnit() = default;

	/// normal constructor
	TokenProcessingUnit(const TokenProcessorPack<TokenProcessorAlgoT> &processor_pack, const int token_queue_limit, const int result_queue_limit) :
			m_processor_pack{processor_pack},
			m_token_queue{token_queue_limit},
			m_result_queue{result_queue_limit}
	{
		// worker thread starts here
		m_worker = std::thread{&TokenProcessingUnit<TokenProcessorAlgoT>::WorkerFunction, this};
	}

	/// copy constructor: default construct
	TokenProcessingUnit(const TokenProcessingUnit&) {}

	/// destructor
	~TokenProcessingUnit()
	{
		// join the worker thread if necessary (is joining in destructor legitimate?)
		if (m_worker.joinable())
			m_worker.join();
	}

//overloaded operators
	/// asignment operator: disabled
	TokenProcessingUnit& operator=(const TokenProcessingUnit&) = delete;
	TokenProcessingUnit& operator=(const TokenProcessingUnit&) const = delete;

//member functions
	/// shut down the unit (no more tokens to be added)
	void ShutDown()
	{
		m_token_queue.ShutDown();
	}

	/// wait until the unit's thread ends
	void WaitUntilUnitStops()
	{
		if (m_worker.joinable())
			m_worker.join();
	}

	/// try insert wrapper for insert queue
	bool TryInsert(std::unique_ptr<TokenT> &insert_token)
	{
		// make sure there is a token inside the unique_ptr
		if (insert_token)
			return m_token_queue.TryInsertToken(insert_token);
		else
			return false;
	}

	/// try get result wrapper for result queue
	bool TryGetResult(std::unique_ptr<ResultT> &return_val)
	{
		return m_result_queue.TryGetToken(return_val);
	}

	/// extract results lingering in queue when unit is stopped
	/// returns true if a result was extracted
	bool ExtractFinalResults(std::unique_ptr<ResultT> &return_val)
	{
		assert(!m_worker.joinable() && "ExtractFinalResults() can only be called after WaitUntilUnitStops()!");

		// if there are elements remaining in the queue, grab one
		// this should not block because no threads should be competing for the result queue (worker thread is shut down)
		if (!m_result_queue.IsEmpty())
		{
			m_result_queue.GetToken(return_val);

			return true;
		}
		else
		{
			return !m_result_queue.IsEmpty();
		}
	}

	/// function that lives in a thread and does active work
	void WorkerFunction()
	{
		static_assert(std::is_base_of<TokenProcessorBase<TokenProcessorAlgoT>, TokenProcessorT>::value,
			"Token processor implementation does not derive from the TokenProcessorBase!");

		// relies on template dependency injection to decide the processor algorithm
		TokenProcessorT worker_processor{m_processor_pack};
		std::unique_ptr<TokenT> token_shuttle{};
		std::unique_ptr<ResultT> result_shuttle{};

		// get tokens asynchronously until the queue shuts down (and is empty)
		while(m_token_queue.GetToken(token_shuttle))
		{
			// sanity check: if a token is obtained then it should exist
			assert(token_shuttle);

			worker_processor.Insert(std::move(token_shuttle));

			// sanity check: inserted tokens should not exist here any more
			assert(!token_shuttle);

			// see if the token processor has a result ready, and add it to the result queue if it does
			// possible deadlock: result queue full and this thread stalls on insert, but token queue is full so thread that
			//	removes results is stalled on token insert
			//	ANSWER: inserter should alternate between 'tryinsert()' and 'trygetresult()' whenever they have a new token
			// 		- in practice only TryInsert() and TryGetResult() are exposed to users of the processing unit
			if (worker_processor.TryGetResult(result_shuttle))
			{
				// sanity check: successfully obtained results should exist
				assert(result_shuttle);

				m_result_queue.InsertToken(result_shuttle);

				// sanity check: inserted tokens should not exist
				assert(!result_shuttle);
			}
			else
			{
				// sanity check: unsuccessfully obtained results should not exist
				assert(!result_shuttle);
			}
		}

		// tell token processor there are no more tokens so it can prepare final results
		worker_processor.NotifyNoMoreTokens();

		// obtain final result if it exists
		if (worker_processor.TryGetResult(result_shuttle))
		{
			// sanity check: successfully obtained results should exist
			assert(result_shuttle);

			// force insert result to avoid deadlocks in shutdown procedure
			m_result_queue.InsertToken(result_shuttle, true);

			// sanity check: inserted tokens should not exist
			assert(!result_shuttle);
		}
		else
		{
			// sanity check: unsuccessfully obtained results should not exist
			assert(!result_shuttle);
		}
	}

private:
//member variables
	/// variable pack for initializing the token processor
	const TokenProcessorPack<TokenProcessorAlgoT> m_processor_pack{};
	/// queue of tokens, which are inserted by users
	TokenQueueT m_token_queue{};
	/// queue of results, which are obtained from token processor
	ResultQueueT m_result_queue{};
	/// worker thread that moves tokens from queue to token processor, and gets results from the processor
	std::thread m_worker{};
};


#endif //header guard












