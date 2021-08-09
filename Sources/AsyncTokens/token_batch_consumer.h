// token consumer base class

#ifndef TOKEN_BATCH_CONSUMER_67876789_H
#define TOKEN_BATCH_CONSUMER_67876789_H

//local headers

//third party headers

//standard headers
#include <memory>


template <typename TokenT, typename FinalResultT>
class TokenBatchConsumer
{
//member types
public:
    using token_type = TokenT;
    using final_result_type = FinalResultT;

//constructors
    /// default constructor: disabled
    TokenBatchConsumer() = delete;

    /// normal constructor
    TokenBatchConsumer(const int batch_size, const bool collect_timings) :
        m_batch_size{static_cast<std::size_t>(batch_size)},
        m_collect_timings{collect_timings}
    {
        assert(batch_size > 0);
    }

    /// copy constructor: disabled
    TokenBatchConsumer(const TokenBatchConsumer&) = delete;

//destructor: default
    virtual ~TokenBatchConsumer() = default;

//overloaded operators
    /// asignment operator: disabled
    TokenBatchConsumer& operator=(const TokenBatchConsumer&) = delete;
    TokenBatchConsumer& operator=(const TokenBatchConsumer&) const = delete;

//member functions
    /// get batch size (number of tokens in each batch)
    virtual std::size_t GetBatchSize() final { return m_batch_size; }

    /// consume a token
    virtual void ConsumeToken(std::unique_ptr<TokenT> input_token, const std::size_t index_in_batch) final
    {
        TSIntervalTimer::time_pt_t interval_start_time{};

        // start initial timer
        if (m_collect_timings)
            interval_start_time = m_timer.GetTime();

        ConsumeTokenImpl(std::move(input_token), index_in_batch);

        // add interval and update start time
        if (m_collect_timings)
            m_timer.AddInterval(interval_start_time);
    }

    /// get final result; should also reset the consumer to restart consuming
    virtual std::unique_ptr<FinalResultT> GetFinalResult() = 0;

    /// get interval report for how much time was spent producing each batch (resets timer)
    /// TimeUnit must be e.g. std::chrono::milliseconds
    template <typename TimeUnit>
    TSIntervalReport<TimeUnit> GetTimingReport()
    {
        auto report{m_timer.GetReport<TimeUnit>()};

        m_timer.Reset();

        return report;
    }

protected:
    /// consume a token (implementation)
    virtual void ConsumeTokenImpl(std::unique_ptr<TokenT> input_token, const std::size_t index_in_batch) = 0;

private:
//member variables
    /// batch size (number of tokens per batch)
    const std::size_t m_batch_size{};
    /// whether to collecting timing info
    const bool m_collect_timings{};

    /// interval timer (collects the time it takes to produce each batch of tokens)
    TSIntervalTimer m_timer{};
};


#endif //header guard















