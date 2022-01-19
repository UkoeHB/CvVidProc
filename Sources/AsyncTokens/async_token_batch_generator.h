// async token generator base class

#ifndef ASYNC_TOKEN_BATCH_GENERATOR_3468986_H
#define ASYNC_TOKEN_BATCH_GENERATOR_3468986_H

//local headers
#include "token_batch_generator.h"
#include "token_generator_algo.h"
#include "token_queue.h"

//third party headers

//standard headers
#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>


template <typename TokenGeneratorAlgoT>
class AsyncTokenBatchGenerator final : public TokenBatchGenerator<typename TokenGeneratorAlgoT::token_type>
{
//member types
public:
    using TokenT = typename TokenGeneratorAlgoT::token_type;
    using BatchT = typename TokenGeneratorAlgoT::token_set_type;
    using PPackSetT = std::vector<TokenGeneratorPack<TokenGeneratorAlgoT>>;

//constructors
    /// default constructor: disabled
    AsyncTokenBatchGenerator() = delete;

    /// normal constructor
    AsyncTokenBatchGenerator(const int batch_size, const bool collect_timings, const int max_queue_size) :
        TokenBatchGenerator<TokenT>{batch_size, collect_timings},
        m_token_queue{max_queue_size}
    {
        static_assert(std::is_base_of<TokenGeneratorAlgo<TokenGeneratorAlgoT, TokenT>, TokenGeneratorAlgoT>::value,
            "Async token generator algo does not derive from TokenGeneratorAlgo!");
    }

    /// copy constructor: disabled
    AsyncTokenBatchGenerator(const AsyncTokenBatchGenerator&) = delete;

//destructor: default
    /// note: if deleted while threads are running, it will make the program crash (e.g. not exception safe)
    ///  this is by design - fail fast instead of hang on potentially stuck thread
    virtual ~AsyncTokenBatchGenerator() = default;

//overloaded operators
    /// asignment operator: disabled
    AsyncTokenBatchGenerator& operator=(const AsyncTokenBatchGenerator&) = delete;

//member functions
    /// reset the token generator so it can be reused
    /// note: does not repopulate the generator algo's parameter pack -> must call StartGenerator() again
    virtual void ResetGenerator() override
    {
        // expect all tokens to be cleared out
        assert(m_token_queue.IsEmpty());

        // expect all workers to be dead
        assert(m_active_workers.load() == 0);

        // join all the threads
        for (auto &worker : m_workers)
        {
            if (worker.joinable())
                worker.join();
        }

        // remove all dead workers
        m_workers = std::vector<std::thread>{};
    }

    /// start the generator with worker parameter packs
    void StartGenerator(std::vector<TokenGeneratorPack<TokenGeneratorAlgoT>> processor_packs)
    {
        // expect no workers
        assert(!m_workers.size());
        // expect no tokens left over from previous run
        assert(m_token_queue.IsEmpty());

        // expect some inputs
        assert(processor_packs.size());

        // prep workers
        m_workers.reserve(processor_packs.size());
        m_active_workers.store(processor_packs.size());

        // make a worker for all the packs
        for (std::size_t i{0}; i < processor_packs.size(); i++)
        {
            // create worker
            auto worker{std::make_unique<TokenGeneratorAlgoT>(std::move(processor_packs[i]))};

            // start worker thread
            m_workers.emplace_back(std::thread{&AsyncTokenBatchGenerator<TokenGeneratorAlgoT>::WorkerFunction, this, std::move(worker)});
        }
    }

protected:
    /// get token set from generator; should return empty vector when no more token sets to get
    virtual BatchT GetTokenSetImpl() override
    {
        // expect there to be workers
        assert(m_workers.size());

        // get a batch from the queue
        BatchT new_batch{};
        m_token_queue.GetToken(new_batch);

        // note: generator is considered 'done generating' if TokenQueue::GetToken does not return anything
        return new_batch;
    }

    void WorkerFunction(std::unique_ptr<TokenGeneratorAlgoT> worker)
    {
        BatchT batch_shuttle{};

        // obtain batches from the worker until it is done
        while (true)
        {
            batch_shuttle = worker->GetTokenSet();

            // if no result returned, this worker should be shut down
            if (!batch_shuttle.size())
            {
                m_active_workers--;

                // shut down queue when all workers are dead
                if (m_active_workers.load() <= 0)
                    m_token_queue.ShutDown();

                break;
            }

            m_token_queue.InsertToken(batch_shuttle);

            assert(batch_shuttle.size() == 0);
        }
    }

private:
//member variables
    /// worker threads (for generating tokens)
    std::vector<std::thread> m_workers;
    /// keep track of how many active workers there are
    std::atomic_int m_active_workers;

    /// queue for collecting generated token batches
    TokenQueue<BatchT> m_token_queue{};
};


#endif //header guard















