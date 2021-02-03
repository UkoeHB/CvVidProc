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
// collect timings over an interval (thread safe)
///
class TSIntervalTimer final
{
//member types
public:
	using time_pt_t = std::chrono::time_point<std::chrono::steady_clock>;

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
		auto current_time{GetTime()};
		m_duration = m_duration.load() + (current_time - start_time);
		m_num_intervals++;

		return current_time;
	}

	/// reset interval timer
	void Reset()
	{
		m_duration = time_pt_t{};
		m_num_intervals = 0;
	}

	/// get interval report
	/// TimeUnit must be e.g. std::chrono::milliseconds
	template <typename TimeUnit>
	TSIntervalReport<TimeUnit> GetReport() const
	{
		auto duration = std::chrono::duration_cast<TimeUnit>(m_duration.load() - time_pt_t{});

		return TSIntervalReport<TimeUnit>{duration, m_num_intervals};
	}


private:
//member variables
	/// duration (no specific units)
	std::atomic<time_pt_t> m_duration{};
	/// number of intervals recorded
	std::atomic<unsigned long> m_num_intervals{};
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















