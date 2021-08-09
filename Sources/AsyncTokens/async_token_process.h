// async batchable token processing process

#ifndef ASYNC_TOKEN_PROCESS_4567898765_H
#define ASYNC_TOKEN_PROCESS_4567898765_H

//local headers
#include "exception_assert.h"
#include "token_batch_generator.h"
#include "token_batch_consumer.h"
#include "token_processor_algo.h"
#include "token_processing_unit.h"
#include "token_queue.h"
#include "ts_interval_timer.h"

//third party headers

//standard headers
#include <cassert>
#include <memory>
#include <mutex>
#include <sstream>
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
template <typename TokenProcessorAlgoT, typename FinalResultT, typename TimingReportUnitT = std::chrono::milliseconds>
class AsyncTokenProcess final
{
//member types
public:
    using TokenT = typename TokenProcessorAlgoT::token_type;
    using ResultT = typename TokenProcessorAlgoT::result_type;
    using PPackSetT = std::vector<TokenProcessorPack<TokenProcessorAlgoT>>;
    using TokenGenT = std::shared_ptr<TokenBatchGenerator<TokenT>>;
    using TokenConsumerT = std::shared_ptr<TokenBatchConsumer<ResultT, FinalResultT>>;

//constructors
    /// default constructor: disabled
    AsyncTokenProcess() = delete;

    /// normal constructor
    AsyncTokenProcess(const int worker_thread_limit,
            const bool synchronous_allowed,
            const bool collect_timings,
            const int token_storage_limit,
            const int result_storage_limit,
            TokenGenT token_generator,
            TokenConsumerT token_consumer) : 
        m_worker_thread_limit{worker_thread_limit},
        m_synchronous_allowed{synchronous_allowed},
        m_token_storage_limit{token_storage_limit},
        m_result_storage_limit{result_storage_limit},
        m_collect_timings{collect_timings},
        m_token_generator{token_generator},
        m_token_consumer{token_consumer}
    {
        static_assert(std::is_base_of<TokenProcessorAlgo<TokenProcessorAlgoT, TokenT, ResultT>, TokenProcessorAlgoT>::value,
            "Token processor implementation does not derive from the TokenProcessorAlgo!");

        EXCEPTION_ASSERT(m_token_generator);
        EXCEPTION_ASSERT(m_token_consumer);

        if (m_token_generator)
            m_batch_size = m_token_generator->GetBatchSize();

        EXCEPTION_ASSERT(m_batch_size > 0);
        EXCEPTION_ASSERT(m_batch_size <= m_worker_thread_limit);
        EXCEPTION_ASSERT(m_batch_size == m_token_consumer->GetBatchSize());

        if (m_collect_timings)
            m_unit_timing_reports.resize(m_batch_size);
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
        // only one thread can use this object at a time
        std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};

        if (!lock.owns_lock())
        {
            assert(false && "async token process can only be run from one thread at a time!");

            return std::unique_ptr<FinalResultT>{};
        }

        // make sure processor packs are the right size
        assert(processing_packs.size() == m_batch_size);

        // spawn set of processing units (creates threads)
        std::vector<TokenProcessingUnit<TokenProcessorAlgoT>> processing_units{};
        processing_units.reserve(m_batch_size);

        for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
        {
            // according to https://stackoverflow.com/questions/5410035/when-does-a-stdvector-reallocate-its-memory-array
            // this will not reallocate the vector unless the .reserve() amount is exceeded, so it should be thread safe
            // the processing units will run synchronously if there is only one token being passed around at a time (and synch mode allowed)
            processing_units.emplace_back(m_synchronous_allowed && m_batch_size == 1,
                m_collect_timings,
                m_token_storage_limit,
                m_result_storage_limit);

            // start the unit's thread
            processing_units[unit_index].Start(std::move(processing_packs[unit_index]));
        }

        // consume tokens until no more are generated
        std::vector<std::unique_ptr<TokenT>> token_set_shuttle{};
        std::unique_ptr<ResultT> result_shuttle{};
        TSIntervalTimer::time_pt_t interval_start_time{};

        // start initial timer
        if (m_collect_timings)
            interval_start_time = m_timer.GetTime();

        while (true)
        {
            // get token set or leave if no more will be created
            token_set_shuttle = m_token_generator->GetTokenSet();

            if (!token_set_shuttle.size())
                break;

            // sanity check: token generator should provide expected number of tokens
            assert(token_set_shuttle.size() == m_batch_size);

            // pass token set to processing units
            // it spins through 'try' functions to avoid deadlocks between token and result queues
            // note: will go to sleep if any unit is blocked (can't insert token or remove result)
            std::size_t remaining_tokens{m_batch_size};
            std::size_t last_unit_stuck_on_full{m_batch_size};
            while (remaining_tokens > 0)
            {
                // recount the number of uninserted tokens each round
                remaining_tokens = 0;

                // reset last unit stuck on full
                last_unit_stuck_on_full = m_batch_size;

                // iterate through token set to empty it
                for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
                {
                    // try to insert the token to its processing unit (if it needs to be inserted)
                    if (token_set_shuttle[unit_index])
                    {
                        TokenQueueCode insert_result{processing_units[unit_index].TryInsert(token_set_shuttle[unit_index])};

                        if (insert_result == TokenQueueCode::Success)
                        {
                            // sanity check - successful insertions should remove the token
                            assert(!token_set_shuttle[unit_index]);
                        }
                        else
                        {
                            remaining_tokens++;

                            if (insert_result == TokenQueueCode::QueueFull)
                                last_unit_stuck_on_full = unit_index;
                        }
                    }

                    // try to get a result from the processing unit
                    if (processing_units[unit_index].TryGetResult(result_shuttle) == TokenQueueCode::Success)
                    {
                        // sanity check: if a result is obtained, it should exist
                        assert(result_shuttle);

                        // consume the result
                        m_token_consumer->ConsumeToken(std::move(result_shuttle), unit_index);
                    }
                }

                // sleep this thread if insert failed for any unit
                if (last_unit_stuck_on_full < m_batch_size)
                    processing_units[last_unit_stuck_on_full].WaitForUnblockingEvent();
            }

            // add interval and update start time
            if (m_collect_timings)
                interval_start_time = m_timer.AddInterval(interval_start_time);
        }

        // shut down all processing units (no more tokens)
        for (auto &unit : processing_units)
        {
            unit.ShutDown();
        }

        // wait for all units to shut down, and clear out their remaining results
        // must spin through the units until they are done producing results
        std::size_t remaining_alive{m_batch_size};
        std::size_t last_unit_still_alive{m_batch_size};
        while (remaining_alive > 0)
        {
            // recount the number of units still alive each round
            remaining_alive = 0;

            // reset last unit still alive
            last_unit_still_alive = m_batch_size;

            // iterate through units trying to stop them and checking if they have results
            for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
            {
                // try to stop the unit
                if (!processing_units[unit_index].TryStop())
                {
                    // try to get a result from the processing unit
                    if (processing_units[unit_index].TryGetResult(result_shuttle) == TokenQueueCode::Success)
                    {
                        // sanity check: if a result is obtained, it should exist
                        assert(result_shuttle);

                        // consume the result
                        m_token_consumer->ConsumeToken(std::move(result_shuttle), unit_index);
                    }

                    remaining_alive++;
                    last_unit_still_alive = unit_index;
                }
                else
                {
                    // get timing report from unit when it is stopped (only the first time)
                    if (m_collect_timings &&
                        m_unit_timing_reports[unit_index].num_intervals == 0)
                    {
                        std::lock_guard<std::mutex> lock{m_unit_timing_mutex};

                        // weird syntax! see https://stackoverflow.com/questions/3786360/confusing-template-error
                        m_unit_timing_reports[unit_index] = processing_units[unit_index].template GetTimingReport<TimingReportUnitT>();
                    }
                }
            }

            // sleep this thread if waiting for the result queue of any unit
            if (last_unit_still_alive < m_batch_size)
                processing_units[last_unit_still_alive].WaitForResult();
        }

        // get final result (before resetting generator for safety/proper order of events)
        auto final_result{m_token_consumer->GetFinalResult()};

        // reset the token generator
        m_token_generator->ResetGenerator();

        // return final result
        return final_result;
    }

    /// get timing information as a string
    std::string GetTimingInfoAndResetTimer()
    {
        if (!m_collect_timings)
            return "";

        std::string unit{TimeUnitStr<TimingReportUnitT>()};
        std::string str{};

        // timing info for overall process
        auto batch_timing{m_timer.GetReport<TimingReportUnitT>()};

        if (!batch_timing.num_intervals)
            return "";

        str += "Batch loading: ";
        {
            std::ostringstream ss;
            ss << batch_timing.total_time.count();
            str += ss.str();
        }
        str += " " + unit + " (";
        {
            std::ostringstream ss;
            ss << batch_timing.num_intervals;
            str += ss.str();
        }
        str += " batches; ";
        {
            std::ostringstream ss;
            ss << (batch_timing.total_time / batch_timing.num_intervals).count();
            str += ss.str();
        }
        str += " " + unit + " avg) on time between each generated batch\n";

        // timing info for token generator
        if (m_token_generator)
        {
            auto generator_timing{m_token_generator->template GetTimingReport<TimingReportUnitT>()};

            str += "Batch gen: ";
            {
                std::ostringstream ss;
                ss << generator_timing.total_time.count();
                str += ss.str();
            }
            str += " " + unit + " (";
            {
                std::ostringstream ss;
                ss << generator_timing.num_intervals;
                str += ss.str();
            }
            str += " batches; ";
            {
                std::ostringstream ss;
                if (generator_timing.num_intervals)
                    ss << (generator_timing.total_time / generator_timing.num_intervals).count();
                else
                    ss << 0;
                str += ss.str();
            }
            str += " " + unit + " avg) on generating batches\n";
        }

        // timing info for token consumer
        if (m_token_consumer)
        {
            auto consumer_timing{m_token_consumer->template GetTimingReport<TimingReportUnitT>()};

            str += "Result consume: ";
            {
                std::ostringstream ss;
                ss << consumer_timing.total_time.count();
                str += ss.str();
            }
            str += " " + unit + " (";
            {
                std::ostringstream ss;
                ss << consumer_timing.num_intervals;
                str += ss.str();
            }
            str += " tokens; ";
            {
                std::ostringstream ss;
                if (consumer_timing.num_intervals)
                    ss << (consumer_timing.total_time / consumer_timing.num_intervals).count();
                else
                    ss << 0;
                str += ss.str();
            }
            str += " " + unit + " avg) on handling results\n";
        }

        // timing info for each processing unit
        std::lock_guard<std::mutex> lock{m_unit_timing_mutex};

        assert(m_unit_timing_reports.size() == m_batch_size);

        for (std::size_t unit_index{0}; unit_index < m_batch_size; unit_index++)
        {
            auto report{m_unit_timing_reports[unit_index]};

            if (!report.num_intervals)
                continue;

            str += "Unit [";
            {
                std::ostringstream ss;
                ss << unit_index + 1;
                str += ss.str();
            }
            str += "]: ";
            {
                std::ostringstream ss;
                ss << report.total_time.count();
                str += ss.str();
            }
            str += " " + unit + " (";
            {
                std::ostringstream ss;
                ss << report.num_intervals;
                str += ss.str();
            }
            str += " tokens; ";
            {
                std::ostringstream ss;
                if (report.num_intervals)
                    ss << (report.total_time / report.num_intervals).count();
                else
                    ss << 0;
                str += ss.str();
            }
            str += " " + unit + " avg) on ingesting tokens in workers\n";
        }

        // reset timer
        m_timer.Reset();

        // reset timing reports
        m_unit_timing_reports.resize(m_batch_size);

        return str;
    }

private:
//member variables
    /// max number of worker threads allowed
    const int m_worker_thread_limit{};
    /// whether the token process can be run synchronously or not
    ///  if there is only one worker thread then it makes sense to run synchronously, unless token generation/consumption
    ///  should be concurrent with token processing
    const bool m_synchronous_allowed{};
    /// if timings should be collected
    const bool m_collect_timings{};

    /// max number of tokens that can be stored in each processing unit
    const int m_token_storage_limit{};
    /// max number of results that can be stored in each processing unit
    const int m_result_storage_limit{};
    /// batch size (number of tokens per batch)
    std::size_t m_batch_size{0};

    /// token set generator
    TokenGenT m_token_generator{};
    /// token consumer
    TokenConsumerT m_token_consumer{};

    /// mutex in case this object is used asynchronously
    std::mutex m_mutex;
    /// mutex for unit timing reports
    std::mutex m_unit_timing_mutex;

    /// interval timer (collects the time it takes to process each batch of tokens)
    TSIntervalTimer m_timer{};
    /// timing reports from all the processing units
    std::vector<TSIntervalReport<TimingReportUnitT>> m_unit_timing_reports{};
};


#endif //header guard
















