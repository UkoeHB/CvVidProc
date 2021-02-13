// token set generator algorithm base class

#ifndef TOKEN_GENERATOR_ALGO_5982832_H
#define TOKEN_GENERATOR_ALGO_5982832_H

//local headers
#include "token_batch_generator.h"

//third party headers

//standard headers


/// by default, the processor pack is empty; add specializations for specific content
/// WARNING: if you want to put a pointer in the pack, only use std::unique_ptr to avoid undefined behavior
/// - note: packs are passed around with move constructors and move assignment operators until they reach TokenGeneratorAlgo
template <typename AlgoT>
struct TokenGeneratorPack final
{};

////
// interface for token processing algorithms
///
template <typename AlgoT, typename TokenT>
class TokenGeneratorAlgo
{
public:
//member types
	using token_type = TokenT;
	using token_set_type = typename TokenBatchGenerator<TokenT>::token_set_type;

//constructors
	/// default constructor: disabled
	TokenGeneratorAlgo() = delete;

	/// normal constructor
	TokenGeneratorAlgo(TokenGeneratorPack<AlgoT> processor_pack) : m_pack{std::move(processor_pack)}
	{}

	/// copy constructor: disabled
	TokenGeneratorAlgo(const TokenGeneratorAlgo&) = delete;

//destructor
	virtual ~TokenGeneratorAlgo() = default;

//overloaded operators
	/// copy assignment operators: disabled
	TokenGeneratorAlgo& operator=(const TokenGeneratorAlgo&) = delete;
	TokenGeneratorAlgo& operator=(const TokenGeneratorAlgo&) const = delete;

//member functions
	/// try to get a token set
	virtual token_set_type GetTokenSet() = 0;

protected:
//member variables
	TokenGeneratorPack<AlgoT> m_pack{};
};




#endif //header guard



















