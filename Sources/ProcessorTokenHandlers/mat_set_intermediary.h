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
			const bool collect_timings,
			const int max_shuttle_queue_size) :
		// output batch size is 1
		TokenProcessIntermediary{batch_size, 1, collect_timings, max_shuttle_queue_size}
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

		// send a batch
		// note: only need to call this at most once per invocation of ConsumeTokenImpl
		//  because at most one batch can be ready at this point
		SendABatch();
	}

	/// clean up remaining tokens (unique ptr return type is an API requirement)
	virtual std::unique_ptr<bool> GetFinalResultImpl() override
	{
		// clear out any remaining elements carelessly
		// this should only cause 'out of order' problems if ConsumeTokenImpl caller sent
		// uneven numbers of tokens to each batch index
		while (SendABatch()) {}

		return std::make_unique<bool>(true);
	}

private:
	/// send a batch in
	/// warning: calling this carelessly can cause tokens passed to the generator to be out of order
	bool SendABatch()
	{
		std::vector<cv::Mat> new_batch{};
		new_batch.reserve(m_elements.size());

		// assemble batch from first element in each batch-position (if available)
		for (auto &element_list : m_elements)
		{
			// ignore empty lists
			if (element_list.empty())
				continue;

			new_batch.emplace_back(std::move(element_list.front()));

			element_list.pop_front();
		}

		if (new_batch.size() == 0)
			return false;

		assert(new_batch.size() <= m_elements.size());

		// the batch is considered 'one output token', so it must be wrapped for TokenBatchGenerator
		std::vector<std::unique_ptr< std::vector<cv::Mat> >> out_token{};
		out_token.emplace_back(std::make_unique<std::vector<cv::Mat>>(std::move(new_batch)));

		// send the token to the generator so the second AsyncTokenProcess can use it
		AddNextBatch(out_token);

		return true;
	}

//member variables
	/// store batch elements until they are ready to be used
	std::vector<std::list<cv::Mat>> m_elements{};
};


#endif //header guard















