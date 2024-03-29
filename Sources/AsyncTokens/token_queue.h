// queue for passing tokens between threads

#ifndef TOKEN_QUEUE_03944823_H
#define TOKEN_QUEUE_03944823_H

//local headers

//third party headers

//standard headers
#include <cassert>
#include <condition_variable>
#include <list>
#include <mutex>


/// return codes
enum class TokenQueueCode
{
    Success,
    LockFail,
    QueueFull,
    QueueEmpty,
    ShutDown,
    GeneralFail
};

template <typename T>
class TokenQueue final
{
//constructors
public:
    /// default constructor: default
    TokenQueue() = default;

    /// normal constructor
    TokenQueue(const int max_queue_size) :
            m_max_queue_size{max_queue_size > 0 ? static_cast<std::size_t>(max_queue_size) : static_cast<std::size_t>(-1)}  
    {}

    /// copy constructor
    TokenQueue(const TokenQueue& queue) : m_max_queue_size{queue.m_max_queue_size}
    {}

//destructor: not needed (final class)

//overloaded operators
    /// copy assignment operator: disabled
    TokenQueue& operator=(const TokenQueue&) = delete;

//member functions
    /// indicate the queue is shutting down (no more tokens to be added)
    void ShutDown()
    {
        {
            std::lock_guard<std::mutex> lock{m_mutex};

            m_shutting_down = true;
        }

        // notify all threads in case they are stuck waiting for a token or trying to insert
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
    TokenQueueCode InsertToken(T &token, bool force_insert = false)
    {
        // lock the queue
        std::unique_lock<std::mutex> lock{m_mutex};

        // wait until the token queue is open
        while (!force_insert && !QueueOpenImpl())
        {
            // if shutting down then can no longer insert a token, and don't want to hang the inserting thread
            if (m_shutting_down)
                return TokenQueueCode::ShutDown;

            m_condvar_fill.wait(lock);
        }

        // once a token can be inserted, insert it
        return TryInsertTokenImpl(token, std::move(lock), force_insert);
    }

    /// try to insert a token; returns false if token can't be inserted (or can't acquire mutex)
    /// may mutate the token passed in if insertion succeeds
    TokenQueueCode TryInsertToken(T &token)
    {
        // try to lock the queue
        std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};

        // try to insert the token
        return TryInsertTokenImpl(token, std::move(lock), false);
    }

    /// get token from one of the queues; hangs if no tokens available
    TokenQueueCode GetToken(T &return_token)
    {
        // lock the queue
        std::unique_lock<std::mutex> lock{m_mutex};

        // wait until a token is available, or until the queue shuts down
        while (m_tokenqueue.empty())
        {
            // only return false when there are no tokens, and won't be more
            if (m_shutting_down)
                return TokenQueueCode::ShutDown;

            m_condvar_gettoken.wait(lock);
        }

        // try to get the token
        return TryGetTokenImpl(return_token, std::move(lock));
    }

    /// try to get token from the queue; returns false if no tokens available (or can't acquire mutex)
    TokenQueueCode TryGetToken(T &return_token)
    {
        // try to lock the queue
        std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};

        // try to get the token
        return TryGetTokenImpl(return_token, std::move(lock));
    }

    /// check if queue is empty
    bool IsEmpty()
    {
        // lock the queue
        std::lock_guard<std::mutex> lock{m_mutex};

        // see if queue is empty
        return m_tokenqueue.empty();
    }

    /// check if queue is open
    bool QueueOpen()
    {
        // lock the queue
        std::lock_guard<std::mutex> lock{m_mutex};

        // see if queue is empty
        return QueueOpenImpl();
    }

private:
    /// try to insert token to queue
    /// allows force inserting to avoid deadlocks in some cases (use with caution)
    TokenQueueCode TryInsertTokenImpl(T &token, std::unique_lock<std::mutex> lock, bool force_insert)
    {
        // expect to own the lock by this point
        if (!lock.owns_lock())
            return TokenQueueCode::LockFail;

        // if shutting down then can no longer insert a token unless forced
        if (!force_insert && m_shutting_down)
            return TokenQueueCode::ShutDown;

        // expect the queue to be open at this point
        if (!force_insert && !QueueOpenImpl())
            return TokenQueueCode::QueueFull;

        // insert the token
        m_tokenqueue.emplace_back(std::move(token));

        // unlock the lock so condition_var notification doesn't cause expensive collisions
        lock.unlock();

        // notify anyone waiting
        m_condvar_gettoken.notify_all();

        return TokenQueueCode::Success;
    }

    /// try to get token from the queue
    TokenQueueCode TryGetTokenImpl(T &return_token, std::unique_lock<std::mutex> lock)
    {
        // expect to own the lock by this point
        if (!lock.owns_lock())
            return TokenQueueCode::LockFail;

        // expect the queue to not be empty at this point
        if (m_tokenqueue.empty())
            return TokenQueueCode::QueueEmpty;

        // get the oldest token from the queue
        return_token = std::move(m_tokenqueue.front());
        m_tokenqueue.pop_front();
        
        // unlock the lock so condition_var notification doesn't cause expensive collisions
        lock.unlock();

        // notify any inserters waiting for a full queue
        m_condvar_fill.notify_all();

        return TokenQueueCode::Success;
    }

    /// see if queue is open; not thread-safe since it should only be used by thread-safe member functions
    bool QueueOpenImpl() const
    {
        assert(m_max_queue_size && "can't use default constructed queue!");

        return m_tokenqueue.size() < m_max_queue_size;
    }

//member variables
private: 
    /// queue of tokens
    std::list<T> m_tokenqueue{};
    /// max number of tokens the queue can store; queue size <=0 means unlimited
    const std::size_t m_max_queue_size{};
    /// mutex for accessing the queue
    std::mutex m_mutex;
    /// help threads wait for tokens to appear
    std::condition_variable m_condvar_gettoken;
    /// help threads wait until there is room to fill queue
    std::condition_variable m_condvar_fill;
    /// indicate the queue is shutting down (no more tokens will be added)
    bool m_shutting_down{false};
};


#endif  //header guard













