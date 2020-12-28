// token processor algorithm base class

#ifndef TOKEN_PROCESSOR_ALGO_BASE_039587849_H
#define TOKEN_PROCESSOR_ALGO_BASE_039587849_H

//local headers

//third party headers

//standard headers
#include <memory>


/// by default, the processor pack is empty; add specializations for specific content
/// WARNING: if you want to put a pointer in the pack, only use std::unique_ptr to avoid undefined behavior
/// - note: packs are passed around with move constructors and move assignment operators until they reach TokenProcessorAlgoBase
template <typename AlgoT>
struct TokenProcessorPack final
{};

////
// interface for token processing algorithms
///
template <typename AlgoT, typename TokenT, typename ResultT>
class TokenProcessorAlgoBase
{
public:
//member types
	using token_type = TokenT;
	using result_type = ResultT;

//constructors
	/// default constructor: disabled
	TokenProcessorAlgoBase() = delete;

	/// normal constructor
	TokenProcessorAlgoBase(TokenProcessorPack<AlgoT> processor_pack) : m_processor_pack{std::move(processor_pack)}
	{}

	/// copy constructor: disabled
	TokenProcessorAlgoBase(const TokenProcessorAlgoBase&) = delete;

//destructor
	virtual ~TokenProcessorAlgoBase() = default;

//overloaded operators
	/// copy assignment operators: disabled
	TokenProcessorAlgoBase& operator=(const TokenProcessorAlgoBase&) = delete;
	TokenProcessorAlgoBase& operator=(const TokenProcessorAlgoBase&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<TokenT>) = 0;

	/// try to get a processing result; should return empty ptr on failure
	virtual std::unique_ptr<ResultT> TryGetResult() = 0;

	/// get notified there won't be any more tokens
	virtual void NotifyNoMoreTokens() = 0;

protected:
//member variables
	TokenProcessorPack<AlgoT> m_processor_pack{};
};




#endif //header guard



















