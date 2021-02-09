// takes in cv::Mats and collects them together

#ifndef MAT_SET_INTERMEDIARY_43431213_H
#define MAT_SET_INTERMEDIARY_43431213_H

//local headers
#include "token_process_intermediary.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <memory>
#include <vector>


/// designed to stand between an AsyncTokenProcess creating batches of cv::Mats
///  and an AsyncTokenProcess consuming vectors of cv::Mats that are treated as single tokens
class MatSetIntermediary final : public TokenProcessIntermediary<cv::Mat, std::vector<cv::Mat>, bool>
{
//member types
public:
//constructors
	/// default constructor: disabled
	MatSetIntermediary() = delete;

	/// normal constructor
	MatSetIntermediary(const int batch_size,
			const int max_shuttle_queue_size,
			const bool collect_timings) :
		// output batch size is 1
		TokenProcessIntermediary{batch_size, 1, max_shuttle_queue_size, collect_timings}
	{
		m_elements.resize(batch_size);
	}

	/// copy constructor: disabled
	MatSetIntermediary(const MatSetIntermediary&) = delete;

//destructor: default
	virtual ~MatSetIntermediary() = default;

//overloaded operators
	/// asignment operator: disabled
	MatSetIntermediary& operator=(const MatSetIntermediary&) = delete;
	MatSetIntermediary& operator=(const MatSetIntermediary&) const = delete;

//member functions
	/// consume a token from first process
	virtual void ConsumeTokenImpl(std::unique_ptr<cv::Mat> input_token, const std::size_t index_in_batch) override
	{
		assert(index_in_batch < m_elements.size());

		// get the Mat out of the unique_ptr so we can transition to vector of Mats
		m_elements[index_in_batch].emplace_back(std::move(*(input_token.release())));

		// check if a full batch can be assembled
		for (const auto &element_list : m_elements)
		{
			if (element_list.empty())
				return;
		}

		// add a batch
		std::vector<cv::Mat> new_batch{};
		new_batch.reserve(m_elements.size());

		// assemble batch from first element in each batch-position
		for (const auto &element_list : m_elements)
		{
			new_batch.emplace_back(std::move(element_list.front()));

			element_list.pop_front();
		}

		assert(new_batch.size() == m_elements.size());

		// the batch is considered 'one output token', so it must be wrapped for TokenBatchGenerator
		std::vector<std::unique_ptr< std::vector<cv::Mat> >> out_token{};
		out_token.emplace_back(std::make_unique<std::vector<cv::Mat>>(std::move(new_batch)));

		// send the token to the generator so the second AsyncTokenProcess can use it
		AddNextBatch(out_token);
	}

	/// get final result and reset the elements (unique ptr is an API requirement)
	virtual std::unique_ptr<bool> GetFinalResultImpl() override
	{
		// check that the elements have been cleared out
		bool elements_cleared{true};

		for (const auto &element_list : m_elements)
		{
			if (!element_list.empty())
			{
				elements_cleared = false;

				// elements remain, so reset first
				m_elements.resize(m_elements.size());

				break;
			}
		}

		return std::make_unique<bool>(elements_cleared);
	}

private:
//member variables
	/// store batch elements until they are ready to be used
	std::vector<std::list<cv::Mat>> m_elements{};
};


#endif //header guard















