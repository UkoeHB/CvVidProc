// token process intermediary base class

#ifndef TOKEN_PROCESS_INTERMEDIARY_765678987_H
#define TOKEN_PROCESS_INTERMEDIARY_765678987_H

//local headers
#include "token_batch_generator.h"
#include "token_batch_consumer.h"

//third party headers

//standard headers
#include <memory>
#include <vector>


/// designed to stand between two AsyncTokenProcesses
/// it consumes the first process's outputs, then creates a new batch for the second process
template <typename InTokenT, typename OutTokenT, typename FinalResultT>
class TokenProcessIntermediary : public TokenBatchConsumer<InTokenT, FinalResultT>, public TokenBatchGenerator<OutTokenT>
{
//member types
public:
	using ConsumerT = TokenBatchConsumer<InTokenT, FinalResultT>;
	using GeneratorT = TokenBatchGenerator<OutTokenT>;

//constructors
	/// default constructor: disabled
	TokenProcessIntermediary() = delete;

	/// normal constructor
	TokenProcessIntermediary(const int consumer_batch_size,
			const int generator_batch_size,
			const int max_shuttle_queue_size) :
		ConsumerT{consumer_batch_size},
		GeneratorT{generator_batch_size},
		m_shuttle_queue{max_shuttle_queue_size}
	{}

	/// copy constructor: disabled
	TokenProcessIntermediary(const TokenProcessIntermediary&) = delete;

//destructor: default
	virtual ~TokenProcessIntermediary() = default;

//overloaded operators
	/// asignment operator: disabled
	TokenProcessIntermediary& operator=(const TokenProcessIntermediary&) = delete;
	TokenProcessIntermediary& operator=(const TokenProcessIntermediary&) const = delete;

//member functions
	/// consume a token from first process; should call AddNextBatch() periodically
	virtual void ConsumeToken(std::unique_ptr<InTokenT> input_token, const std::size_t index_in_batch) = 0;

	/// get final result; should also reset the child object in case a new production run is started
	virtual std::unique_ptr<FinalResultT> GetFinalResultImpl() = 0;

	/// obligatory override for TokenBatchGenerator
	virtual void ResetGenerator() override {}

	/// wrapper for getting the final result
	virtual std::unique_ptr<FinalResultT> GetFinalResult() override final
	{
		// shut down the shuttle queue (GetFinalResult() should be called after the last ConsumeToken())
		m_shuttle_queue.ShutDown();

		return GetFinalResultImpl();
	}

	/// add next token set to queue for second process to obtain (blocks)
	void AddNextBatch(std::vector<std::unique_ptr<OutTokenT>> &out_batch)
	{
		m_shuttle_queue.InsertToken(out_batch);
	}

	/// gets a token set when it is available (blocks)
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSet() override final
	{
		std::vector<std::unique_ptr<TokenT>> return_token_set{};

		m_shuttle_queue.GetToken(return_token_set);

		return return_token_set;
	}

private:
//member variables
	/// queue for passing batch of OutTokens to next process
	TokenQueue<std::vector<std::unique_ptr<OutTokenT>>> m_shuttle_queue{};
};


#endif //header guard















