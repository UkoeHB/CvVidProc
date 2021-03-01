// thread safe interval timer

#ifndef TS_INTERVAL_TIMER_79878_H
#define TS_INTERVAL_TIMER_79878_H

//local headers

//third party headers

//standard headers
#include <atomic>
#include <chrono>
#include <string>


template<typename TimeUnit>
struct TSIntervalReport
{
	/// total time spent in each interval
	TimeUnit total_time{};
	/// number of intervals
	unsigned long num_intervals{};
};


////
// collect timings over an interval (thread safe for read vs write)
///
class TSIntervalTimer final
{
//member types
public:
	using time_pt_t = std::chrono::time_point<std::chrono::steady_clock>;

	struct IntervalPair
	{
		/// duration (no specific units)
		time_pt_t time;
		/// number of intervals recorded
		unsigned long intervals;

		///FIX: compiler mismatch
		/// see: https://stackoverflow.com/questions/29483120/program-with-noexcept-constructor-accepted-by-gcc-rejected-by-clang
#ifndef __clang__
		IntervalPair() noexcept = default;
#endif
	};

//constructors
	/// default constructor: default
	TSIntervalTimer() = default;

	/// normal constructor: none

	/// copy constructor: default construct new timer
	TSIntervalTimer(const TSIntervalTimer&) : TSIntervalTimer{}
	{}

//destructor: default

//overloaded operators
	/// asignment operator: disabled
	TSIntervalTimer& operator=(const TSIntervalTimer&) = delete;
	TSIntervalTimer& operator=(const TSIntervalTimer&) const = delete;

//member functions
	/// get current time
	time_pt_t GetTime() const
	{
		return std::chrono::steady_clock::now();
	}

	/// add interval based on input time
	time_pt_t AddInterval(time_pt_t start_time)
	{
		// get current time
		auto current_time{GetTime()};

		// atomically update the record
		//assert(m_interval_record.is_lock_free()); <- should be true on most implementations
		IntervalPair old_record{m_interval_record.load()};
		IntervalPair new_record{};
		do
		{
			new_record.time = old_record.time + (current_time - start_time);
			new_record.intervals = old_record.intervals + 1;
		} while (!m_interval_record.compare_exchange_weak(old_record, new_record));

		return current_time;
	}

	/// reset interval timer
	void Reset()
	{
		// atomically reset the record
		m_interval_record.store(IntervalPair{time_pt_t{}, 0});
	}

	/// get interval report
	/// TimeUnit must be e.g. std::chrono::milliseconds
	template <typename TimeUnit>
	TSIntervalReport<TimeUnit> GetReport() const
	{
		IntervalPair temp_record{m_interval_record.load()};

		auto duration = std::chrono::duration_cast<TimeUnit>(temp_record.time - time_pt_t{});

		return TSIntervalReport<TimeUnit>{duration, temp_record.intervals};
	}

private:
//member variables
	/// atomic interval pair
	std::atomic<IntervalPair> m_interval_record{};
};


/// helper to get the name of a timing unit
template <typename TimeUnit>
std::string TimeUnitStr()
{
	using namespace std::chrono;

	if (std::is_same<TimeUnit, nanoseconds>::value)
		return "ns";
	else if (std::is_same<TimeUnit, microseconds>::value)
		return "us";
	else if (std::is_same<TimeUnit, milliseconds>::value)
		return "ms";
	else if (std::is_same<TimeUnit, seconds>::value)
		return "s";
	else if (std::is_same<TimeUnit, minutes>::value)
		return "m";
	else if (std::is_same<TimeUnit, hours>::value)
		return "h";
	else
		return "?";
}


#endif //header guard















