// processes tokens in a worker thread with the specified algorithm

#ifndef TOKEN_PROCESSING_UNIT_32573592_H
#define TOKEN_PROCESSING_UNIT_32573592_H

//local headers
#include "token_queue.h"
#include "token_processor_algo.h"
#include "ts_interval_timer.h"

//third party headers

//standard headers
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>


////
// expected usage:
// 		- the class TokenProcessorAlgoT must
//			be derived from TokenProcessorAlgo<TokenProcessorAlgoT>
//		note: c++20 contracts would make this easier...
//
// note: should only be handled by one thread
/// 
template <typename TokenProcessorAlgoT>
class __attribute__ ((visibility("hidden"))) TokenProcessingUnit final
{
public:
//member types
	/// get token type from token processor impl
	using TokenT = typename TokenProcessorAlgoT::token_type;
	/// get result type from token processor impl
	using ResultT = typename TokenProcessorAlgoT::result_type;
	/// token queue type
	using TokenQueueT = TokenQueue<std::unique_ptr<TokenT>>;
	/// result queue type
	using ResultQueueT = TokenQueue<std::unique_ptr<ResultT>>;

//constructors
	/// default constructor: default (not meant to be used, just for placeholding)
	TokenProcessingUnit() = default;

	/// normal constructor
	TokenProcessingUnit(const bool synchronous,
			const bool collect_timings,
			const int token_queue_limit,
			const int result_queue_limit) :
		m_synchronous{synchronous},
		m_collect_timings{collect_timings},
		m_token_queue{token_queue_limit},
		m_result_queue{result_queue_limit}
	{}

	/// copy constructor: default construct (do not copy)
	TokenProcessingUnit(const TokenProcessingUnit&) : TokenProcessingUnit{}
	{}

//destructor: none
	/// note: not exception safe, and may crash the program if unit is destroyed before calling
	///  ShutDown() -> WaitUntilUnitStops()
	///  - reason: want to crash fast instead of hanging on a spinning thread

//overloaded operators
	/// asignment operator: disabled
	TokenProcessingUnit& operator=(const TokenProcessingUnit&) = delete;
	TokenProcessingUnit& operator=(const TokenProcessingUnit&) const = delete;

//member functions
	/// start the unit's thread; unit can be restarted once cleaned up properly (ShutDown() called and TryStop() returns true)
	bool Start(TokenProcessorPack<TokenProcessorAlgoT> processor_pack)
	{
		if (!m_worker.joinable() && m_token_queue.IsEmpty() && m_result_queue.IsEmpty())
		{
			m_worker_processor = std::make_unique<TokenProcessorAlgoT>(std::move(processor_pack));

			// start the worker thread if running asynchronously
			if (!m_synchronous)
			{
				m_worker = std::thread{&TokenProcessingUnit<TokenProcessorAlgoT>::WorkerFunction, this};
			}

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
		// synchronous mode
		if (m_synchronous)
		{
			if (m_worker_processor)
			{
				// manually shut down the processor
				m_worker_processor->NotifyNoMoreTokens();
			}
			else
			{
				assert(false && "tried to shut down processing unit in synch mode but processor doesn't exist!");
			}
		}

		// shut down token queue
		m_token_queue.ShutDown();
	}

	/// try to stop the unit; fails when the worker may still have results to give/produce
	bool TryStop()
	{
		if (m_synchronous)
		{
			// can't stop until the worker is done making results
			return !m_worker_processor->HasResults();
		}
		// must not join unless result queue is completely empty to make sure unit owner does not hang on join
		//  if the unit is stuck on inserting a result
		else if (m_worker.joinable())
		{
			if (!m_result_queue.IsShuttingDown() || !m_result_queue.IsEmpty())
				return false;

			m_worker.join();
		}

		return true;
	}

	/// try insert wrapper for insert queue
	TokenQueueCode TryInsert(std::unique_ptr<TokenT> &insert_token)
	{
		// make sure there is a token inside the unique_ptr
		if (insert_token)
		{
			// synchronous mode
			if (m_synchronous)
			{
				if (!m_worker_processor)
				{
					assert(false && "can't insert token in synchronous mode unless unit has been started!");

					return TokenQueueCode::GeneralFail;
				}

				// start timer
				TSIntervalTimer::time_pt_t interval_start_time{};

				if (m_collect_timings)
					interval_start_time = m_timer.GetTime();

				// insert token to processor directly (always works)
				m_worker_processor->Insert(std::move(insert_token));

				// end timer
				if (m_collect_timings)
					m_timer.AddInterval(interval_start_time);

				// sanity check: inserted tokens should not exist here any more
				assert(!insert_token);

				return TokenQueueCode::Success;
			}
			// asynchronous mode
			else
			{
				return m_token_queue.TryInsertToken(insert_token);
			}
		}
		else
			return TokenQueueCode::GeneralFail;
	}

	/// try get result wrapper for result queue
	TokenQueueCode TryGetResult(std::unique_ptr<ResultT> &return_val)
	{
		// sanity check, input should be empty so it isn't destroyed by accident
		assert(!return_val);

		// synchronous mode
		if (m_synchronous)
		{
			// request from processor directly
			return_val = m_worker_processor->TryGetResult();

			return return_val.get() ? TokenQueueCode::Success : TokenQueueCode::GeneralFail;
		}
		// asynchronous mode
		else
		{
			return m_result_queue.TryGetToken(return_val);
		}
	}

	/// wait until either a token can be inserted or a result extracted
	void WaitForUnblockingEvent()
	{
		if (m_synchronous)
			return;

		std::unique_lock<std::mutex> lock{m_unit_unblocking_mutex};

		if (!m_token_queue.QueueOpen() && m_result_queue.IsEmpty())
			m_condvar_unblockingevents.wait(lock);
	}

	/// wait for result queue (useful when all tokens have been inserted but results still being produced)
	void WaitForResult()
	{
		if (m_synchronous)
			return;

		std::unique_lock<std::mutex> lock{m_unit_unblocking_mutex};

		if (!m_result_queue.IsShuttingDown() && m_result_queue.IsEmpty())
			m_condvar_unblockingevents.wait(lock);
	}

	/// get interval report for how much time was spent processing each token (resets timer)
	/// TimeUnit must be e.g. std::chrono::milliseconds
	template <typename TimeUnit>
	TSIntervalReport<TimeUnit> GetTimingReport()
	{
		auto report{m_timer.GetReport<TimeUnit>()};

		m_timer.Reset();

		return report;
	}

private:
	/// function that lives in a thread and does active work
	void WorkerFunction()
	{
		static_assert(std::is_base_of<TokenProcessorAlgo<TokenProcessorAlgoT, TokenT, ResultT>, TokenProcessorAlgoT>::value,
			"Token processor implementation does not derive from the TokenProcessorAlgo!");

		if (!m_worker_processor)
		{
			assert(false && "can't run token processing unit without starting it!");

			return;
		}

		// create shuttles
		std::unique_ptr<TokenT> token_shuttle{};
		std::unique_ptr<ResultT> result_shuttle{};
		bool notify_unblocking{false};

		// prepare timer tracker
		TSIntervalTimer::time_pt_t interval_start_time{};

		// get tokens asynchronously until the queue shuts down (and is empty)
		while(true)
		{
			// getting a token an unblocking event because it might open the queue so the unit's owner
			//  can insert a token (where it otherwise may have been unable to)
			{
				std::lock_guard<std::mutex> lock{m_unit_unblocking_mutex};

				TokenQueueCode token_result{m_token_queue.GetToken(token_shuttle)};

				if (token_result == TokenQueueCode::Success)
					notify_unblocking = true;
				else
					break;
			}

			// notify outside the lock for better performance
			if (notify_unblocking)
			{
				m_condvar_unblockingevents.notify_all();

				notify_unblocking = false;
			}

			// sanity check: if a token is obtained then it should exist
			assert(token_shuttle);

			// start timer
			if (m_collect_timings)
				interval_start_time = m_timer.GetTime();

			m_worker_processor->Insert(std::move(token_shuttle));

			// end timer
			if (m_collect_timings)
				m_timer.AddInterval(interval_start_time);

			// sanity check: inserted tokens should not exist here any more
			assert(!token_shuttle);

			// see if the token processor has a result ready, and add it to the result queue if it does
			// possible deadlock: result queue full and this thread stalls on result insert, but token queue is full
			//	so thread that removes results is stalled on token insert
			//	ANSWER: inserter should alternate between 'tryinsert()' and 'trygetresult()' whenever they have a new token
			// 		- in practice only TryInsert() and TryGetResult() are exposed to users of the processing unit
			result_shuttle = m_worker_processor->TryGetResult();

			if (result_shuttle)
			{
				// adding a result to the result queue is an unblocking event because it lets the unit's owner
				//  do something (get a result out)
				{
					std::lock_guard<std::mutex> lock{m_unit_unblocking_mutex};

					if (m_result_queue.InsertToken(result_shuttle) == TokenQueueCode::Success)
						notify_unblocking = true;
				}

				// notify outside the lock for better performance
				if (notify_unblocking)
				{
					m_condvar_unblockingevents.notify_all();

					notify_unblocking = false;
				}

				// sanity check: inserted tokens should not exist
				assert(!result_shuttle);
			}
		}

		// tell token processor there are no more tokens so it can prepare final results
		m_worker_processor->NotifyNoMoreTokens();

		// obtain final results if they exist
		while (true)
		{
			result_shuttle = m_worker_processor->TryGetResult();

			if (result_shuttle)
			{
				{
					std::lock_guard<std::mutex> lock{m_unit_unblocking_mutex};

					if (m_result_queue.InsertToken(result_shuttle) == TokenQueueCode::Success)
						notify_unblocking = true;
				}

				// notify outside the lock for better performance
				if (notify_unblocking)
					m_condvar_unblockingevents.notify_all();

				// sanity check: inserted tokens should not exist
				assert(!result_shuttle);
			}
			else
				break;
		}

		// shut down result queue (no more results)
		{
			std::lock_guard<std::mutex> lock{m_unit_unblocking_mutex};

			m_result_queue.ShutDown();
		}

		// notify outside the lock for better performance
		m_condvar_unblockingevents.notify_all();
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
	/// token processor algorithm object
	std::unique_ptr<TokenProcessorAlgoT> m_worker_processor{};
	/// interval timer (collects the time it takes to process each token; does not time 'TryGetResult()')
	TSIntervalTimer m_timer{};

	/// mutex for managing unblocking events
	std::mutex m_unit_unblocking_mutex;
	/// condition variable for waiting for unblocking events
	std::condition_variable m_condvar_unblockingevents;

	/// indicates if the unit is running synchronously or asynchronously
	const bool m_synchronous{};
	/// whether to collect timings or not
	const bool m_collect_timings{};
};


#endif //header guard












