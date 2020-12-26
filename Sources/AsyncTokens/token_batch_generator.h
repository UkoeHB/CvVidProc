// token generator base class

#ifndef TOKEN_BATCH_GENERATOR_098989_H
#define TOKEN_BATCH_GENERATOR_098989_H

//local headers

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

//constructors
	/// default constructor: disabled
	TokenBatchGenerator() = delete;

	/// normal constructor
	TokenBatchGenerator(const int batch_size) :
		m_batch_size{static_cast<std::size_t>(batch_size)}
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
	std::size_t GetBatchSize() { return m_batch_size; }

	/// get token set from generator; should return false when no more token sets to get
	virtual std::vector<std::unique_ptr<TokenT>> GetTokenSet() = 0;

	/// reset the token generator so it can be reused
	virtual void ResetGenerator() = 0;

private:
//member variables
	/// batch size (number of tokens per batch)
	const std::size_t m_batch_size{};
};


#endif //header guard















