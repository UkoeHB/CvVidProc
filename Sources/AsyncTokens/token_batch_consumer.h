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

//constructors
	/// default constructor: disabled
	TokenBatchConsumer() = delete;

	/// normal constructor
	TokenBatchConsumer(const int batch_size) :
		m_batch_size{static_cast<std::size_t>(batch_size)}
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
	virtual void ConsumeToken(std::unique_ptr<TokenT> input_token, const std::size_t index_in_batch) = 0;

	/// get final result; should also reset the consumer to restart consuming
	virtual std::unique_ptr<FinalResultT> GetFinalResult() = 0;

private:
//member variables
	/// batch size (number of tokens per batch)
	const std::size_t m_batch_size{};
};


#endif //header guard















