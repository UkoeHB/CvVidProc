// async queue for set of tokens

#ifndef ASYNC_TOKEN_SET_QUEUE_03944823_H
#define ASYNC_TOKEN_SET_QUEUE_03944823_H

//local headers

//third party headers

//standard headers
#include <cassert>
#include <condition_variable>
#include <list>
#include <mutex>
#include <vector>


////
// naive implementation
// used to insert token batches to multiple queues at once
///
template <typename T>
class AsyncTokenSetQueue final
{
//constructors
public: 
	/// default constructor: disabled
	AsyncTokenSetQueue() = delete;

	/// normal constructor
	AsyncTokenSetQueue(const int num_queues, const int max_queue_size) :
			m_num_queues{static_cast<std::size_t>(num_queues)},
			m_max_queue_size{static_cast<std::size_t>(max_queue_size)}	
	{
		m_tokensetqueue.resize(m_num_queues);
	}

	/// copy constructor: disabled
	AsyncTokenSetQueue(const AsyncTokenSetQueue&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	AsyncTokenSetQueue& operator=(const AsyncTokenSetQueue&) = delete;
	AsyncTokenSetQueue& operator=(const AsyncTokenSetQueue&) const = delete;

//member functions
	/// indicate the queue is shutting down (no more tokens to be added)
	void ShutDown()
	{
		std::unique_lock<std::mutex> lock{m_mutex};

		m_shutting_down = true;

		// release lock
		lock.unlock();

		// notify all threads in case they are stuck waiting for a token
		m_condvar_gettoken.notify_all();
	}

	/// inform user the queue won't be gaining more tokens
	bool IsShuttingDown()
	{
		std::lock_guard<std::mutex> lock{m_mutex};

		return m_shutting_down;
	}

	/// insert tokens to all queues
	void InsertAll(std::vector<T> tokens)
	{
		// lock the queue
		std::unique_lock<std::mutex> lock{m_mutex};

		// add tokens
		if (tokens.size() != m_tokensetqueue.size())
			assert(false && "InsertAll() expects token set to match number of queues!");

		for (std::size_t index{0}; index < m_tokensetqueue.size(); ++index)
		{
			// only insert the token when its queue is open
			while (!QueueOpen(index))
			{
				m_condvar_fill.wait(lock);
			}

			m_tokensetqueue[index].emplace_back(std::move(tokens[index]));
		}	

		// release lock
		lock.unlock();

		// notify anyone waiting
		m_condvar_gettoken.notify_all();
	}

	/// get token from one of the queues
	bool GetToken(T &return_token, const int input_index)
	{
		// lock the queue
		std::unique_lock<std::mutex> lock{m_mutex};

		std::size_t index{static_cast<std::size_t>(input_index)};

		if (index < 0 || index >= m_num_queues)
			assert(false && "GetToken() expects input indices to be in-range!");

		// wait until a token is available, or until the queue shuts down
		while (m_tokensetqueue[index].empty())
		{
			// only return false when there are no tokens, and won't be more
			if (m_shutting_down)
				return false;

			m_condvar_gettoken.wait(lock);
		}

		// get the token
		return_token = std::move(m_tokensetqueue[index].front());
		m_tokensetqueue[index].pop_front();

		// see if inserters should be notified (check before releasing lock to avoid race conditions)
		bool notify_inserters{QueueOpen(index)};

		// release lock
		lock.unlock();

		// notify any inserters waiting for a full queue
		if (notify_inserters)
			m_condvar_fill.notify_all();

		return true;
	}

private: 
	/// see if all queues are open; not thread-safe since it should only be used by thread-safe member functions
	bool QueueOpen(const std::size_t index) const
	{
		return m_tokensetqueue[index].size() < m_max_queue_size;
	}

//member variables
private: 
	/// set of token queues
	std::vector<std::list<T>> m_tokensetqueue{};
	/// number of token queues in set
	std::size_t m_num_queues{};
	/// max number of tokens per sub-queue
	std::size_t m_max_queue_size{};
	/// mutex for accessing the queues
	std::mutex m_mutex;
	/// help threads wait for tokens to appear
	std::condition_variable m_condvar_gettoken;
	/// help threads wait until there is room to fill queues
	std::condition_variable m_condvar_fill;
	/// indicate the queue is shutting down (no more tokens will be added)
	bool m_shutting_down{false};
};


#endif	//header guard













