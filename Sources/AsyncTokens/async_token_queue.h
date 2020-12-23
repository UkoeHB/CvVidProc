// async queue for passing tokens between threads

#ifndef ASYNC_TOKEN_QUEUE_03944823_H
#define ASYNC_TOKEN_QUEUE_03944823_H

//local headers

//third party headers

//standard headers
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <vector>


template <typename T>
class AsyncTokenQueue final
{
//constructors
public: 
	/// default constructor: default
	AsyncTokenQueue() = default;

	/// normal constructor
	AsyncTokenQueue(const int max_queue_size) :
			m_max_queue_size{static_cast<std::size_t>(max_queue_size)}	
	{
		// sanity check
		assert(max_queue_size > 0);
	}

	/// copy constructor
	AsyncTokenQueue(const AsyncTokenQueue& queue) : m_max_queue_size(queue.m_max_queue_size)
	{}

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	AsyncTokenQueue& operator=(const AsyncTokenQueue&) = delete;
	AsyncTokenQueue& operator=(const AsyncTokenQueue&) const = delete;

//member functions
	/// indicate the queue is shutting down (no more tokens to be added)
	void ShutDown()
	{
		{
			std::lock_guard<std::mutex> lock{m_mutex};

			m_shutting_down = true;
		}

		// notify all threads in case they are stuck waiting for a token
		m_condvar_gettoken.notify_all();
	}

	/// inform user the queue won't be gaining more tokens
	bool IsShuttingDown()
	{
		std::lock_guard<std::mutex> lock{m_mutex};

		return m_shutting_down;
	}

	/// insert token; hangs if token can't be inserted yet (queue full)
	/// may mutate the token passed in
	bool InsertToken(T &token, bool force_insert = false)
	{
		// lock the queue
		std::unique_lock<std::mutex> lock{m_mutex};

		// wait until the token queue is open
		while (!force_insert && !QueueOpen())
		{
			m_condvar_fill.wait(lock);
		}

		// once a token can be inserted, insert it
		return TryInsertToken(token, std::move(lock), force_insert);
	}

	/// try to insert a token; returns false if token can't be inserted (or can't acquire mutex)
	/// may mutate the token passed in if insertion succeeds
	bool TryInsertToken(T &token)
	{
		// try to lock the queue
		std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};

		// try to insert the token
		return TryInsertToken(token, std::move(lock), false);
	}

	/// get token from one of the queues; hangs if no tokens available
	bool GetToken(T &return_token)
	{
		// lock the queue
		std::unique_lock<std::mutex> lock{m_mutex};

		// wait until a token is available, or until the queue shuts down
		while (m_tokenqueue.empty())
		{
			// only return false when there are no tokens, and won't be more
			if (m_shutting_down)
				return false;

			m_condvar_gettoken.wait(lock);
		}

		// try to get the token
		return TryGetToken(return_token, std::move(lock));
	}

	/// try to get token from the queue; returns false if no tokens available (or can't acquire mutex)
	bool TryGetToken(T &return_token)
	{
		// try to lock the queue
		std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};

		// try to get the token
		return TryGetToken(return_token, std::move(lock));
	}

	/// check if queue is empty
	bool IsEmpty()
	{
		// lock the queue
		std::lock_guard<std::mutex> lock{m_mutex};

		// see if queue is empty
		return m_tokenqueue.empty();
	}

private:
	/// try to insert token to queue
	/// allows force inserting to avoid deadlocks in some cases (use with caution)
	bool TryInsertToken(T &token, std::unique_lock<std::mutex> lock, bool force_insert)
	{
		// expect to own the lock by this point
		if (!lock.owns_lock())
			return false;

		// expect the queue to be open at this point
		if (!force_insert && !QueueOpen())
			return false;

		// insert the token
		m_tokenqueue.emplace_back(std::move(token));

		// unlock the lock so condition_var notification doesn't cause expensive collisions
		lock.unlock();

		// notify anyone waiting
		m_condvar_gettoken.notify_all();

		return true;
	}

	/// try to get token from one of the queues
	bool TryGetToken(T &return_token, std::unique_lock<std::mutex> lock)
	{
		// expect to own the lock by this point
		if (!lock.owns_lock())
			return false;

		// expect the queue to not be empty at this point
		if (m_tokenqueue.empty())
			return false;

		// get the oldest token from the queue
		return_token = std::move(m_tokenqueue.front());
		m_tokenqueue.pop_front();
		
		// unlock the lock so condition_var notification doesn't cause expensive collisions
		lock.unlock();

		// notify any inserters waiting for a full queue
		m_condvar_fill.notify_all();

		return true;
	}

	/// see if all queues are open; not thread-safe since it should only be used by thread-safe member functions
	bool QueueOpen() const
	{
		return m_tokenqueue.size() < m_max_queue_size;
	}

//member variables
private: 
	/// set of token queues
	std::list<T> m_tokenqueue{};
	/// max number of tokens per sub-queue
	const std::size_t m_max_queue_size{};
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













