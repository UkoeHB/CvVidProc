// token generator base class

#ifndef TOKEN_BATCH_GENERATOR_098989_H
#define TOKEN_BATCH_GENERATOR_098989_H

//local headers
#include "ts_interval_timer.h"

//third party headers

//standard headers
#include <cassert>
#include <memory>
#include <vector>


template <typename TokenT>
class TokenBatchGenerator
{
//member types
public:
	using token_type = TokenT;

//constructors
	/// default constructor: disabled
	TokenBatchGenerator() = delete;

	/// normal constructor
	TokenBatchGenerator(const int batch_size, const bool collect_timings) :
		m_batch_size{static_cast<std::size_t>(batch_size)},
		m_collect_timings{collect_timings}
	{
		assert(batch_size > 0);
	}

	/// copy constructor: disabled
	TokenBatchGenerator(const TokenBatchGenerator&) = delete;

//destructor: default
	virtual ~TokenBatchGenerator() = default;

//overloaded operators
	/// asignment operator: disabled
	TokenBatchGenerator& operator=(const TokenBatchGenerator&) = delete;
	TokenBatchGenerator& operator=(const TokenBatchGenerator&) const = delete;

//member functions
	/// get batch size (number of tokens in each batch)
	virtual std::size_t GetBatchSize() final { return m_batch_size; }

	/// get token set from generator; should return empty vector when no more token sets to get
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSet() final
	{
		TSIntervalTimer::time_pt_t interval_start_time{};

		// start initial timer
		if (m_collect_timings)
			interval_start_time = m_timer.GetTime();

		auto ret_val{GetTokenSetImpl()};

		// add interval and update start time (only if tokens were obtained)
		if (m_collect_timings && ret_val.size())
			m_timer.AddInterval(interval_start_time);

		return ret_val;
	}

	/// reset the token generator so it can be reused
	virtual void ResetGenerator() = 0;

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
	/// get token set from generator; should return empty vector when no more token sets to get
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSetImpl() = 0;

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















