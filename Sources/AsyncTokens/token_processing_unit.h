// processes tokens in a worker thread with the specified algorithm

#ifndef TOKEN_PROCESSING_UNIT_32573592_H
#define TOKEN_PROCESSING_UNIT_32573592_H

//local headers
#include "token_queue.h"
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
	using TokenQueueT = TokenQueue<std::unique_ptr<TokenT>>;
	/// result queue type
	using ResultQueueT = TokenQueue<std::unique_ptr<ResultT>>;

//constructors
	/// default constructor: default (not meant to be used, just for placeholding)
	TokenProcessingUnit() = default;

	/// normal constructor
	TokenProcessingUnit(const int token_queue_limit, const int result_queue_limit) :
			m_token_queue{token_queue_limit},
			m_result_queue{result_queue_limit}
	{}

	/// copy constructor: default construct (do not copy)
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
	/// start the unit's thread; unit can be restarted once cleaned up properly (ShutDown() and ExtractFinalResults() used)
	bool Start(TokenProcessorPack<TokenProcessorAlgoT> processor_pack)
	{
		// worker thread starts here
		if (!m_worker.joinable() && m_token_queue.IsEmpty() && m_result_queue.IsEmpty())
		{
			m_processor_pack = std::move(processor_pack);

			m_worker = std::thread{&TokenProcessingUnit<TokenProcessorAlgoT>::WorkerFunction, this};

			return true;
		}
		else
		{
			assert(false && "unit can't be started if already running or left in bad state!");

			return false;
		}
	}

	/// shut down the unit (no more tokens to be added)
	void ShutDown()
	{
		// shut down token queue
		m_token_queue.ShutDown();

		// shut down result queue
		m_result_queue.ShutDown();
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
		assert(m_result_queue.IsShuttingDown() && "ExtractFinalResults() can only be called after ShutDown()!");

		// sanity check; return val shouldn't have a value since it would be destroyed (not responsibility of this object)
		assert(!return_val);

		// if there are elements remaining in the queue, grab one
		// this should not block because the queue should be shutting down, and the worker thread has ended
		return m_result_queue.GetToken(return_val);
	}

private:
	/// function that lives in a thread and does active work
	void WorkerFunction()
	{
		static_assert(std::is_base_of<TokenProcessorBase<TokenProcessorAlgoT>, TokenProcessorT>::value,
			"Token processor implementation does not derive from the TokenProcessorBase!");

		// relies on template dependency injection to decide the processor algorithm
		TokenProcessorT worker_processor{std::move(m_processor_pack)};
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
			// possible deadlock: result queue full and this thread stalls on result insert, but token queue is full
			//	so thread that removes results is stalled on token insert
			//	ANSWER: inserter should alternate between 'tryinsert()' and 'trygetresult()' whenever they have a new token
			// 		- in practice only TryInsert() and TryGetResult() are exposed to users of the processing unit
			result_shuttle = worker_processor.TryGetResult();

			if (result_shuttle)
			{
				m_result_queue.InsertToken(result_shuttle);

				// sanity check: inserted tokens should not exist
				assert(!result_shuttle);
			}
		}

		// tell token processor there are no more tokens so it can prepare final results
		worker_processor.NotifyNoMoreTokens();

		// obtain final result if it exists
		result_shuttle = worker_processor.TryGetResult();

		if (result_shuttle)
		{
			// force insert result to avoid deadlocks in shutdown procedure
			// i.e. where the unit's owner isn't clearing out the queue
			m_result_queue.InsertToken(result_shuttle, true);

			// sanity check: inserted tokens should not exist
			assert(!result_shuttle);
		}
	}

//member variables
	/// variable pack for initializing the token processor
	TokenProcessorPack<TokenProcessorAlgoT> m_processor_pack{};
	/// queue of tokens, which are inserted by users
	TokenQueueT m_token_queue{};
	/// queue of results, which are obtained from token processor
	ResultQueueT m_result_queue{};
	/// worker thread that moves tokens from queue to token processor, and gets results from the processor
	std::thread m_worker{};
};


#endif //header guard












