// token processor template for managing a token processing algorithm

#ifndef TOKEN_PROCESSOR_039587849_H
#define TOKEN_PROCESSOR_039587849_H

//local headers

//third party headers

//standard headers
#include <cassert>
#include <memory>


/// by default, the processor pack is empty; add specializations for specific content
template <typename T>
struct TokenProcessorPack final
{};

////
// interface for TokenProcessor specializations
// expected usage:
// 		- TokenProcessorAlgoT::token_type must be defined
//		- TokenProcessorAlgoT::result_type must be defined
///
template <typename TokenProcessorAlgoT>
class TokenProcessorBase
{
public:
//member types
	using TokenT = typename TokenProcessorAlgoT::token_type;
	using ResultT = typename TokenProcessorAlgoT::result_type;

//constructors
	/// default constructor: disabled
	TokenProcessorBase() = delete;

	/// normal constructor
	TokenProcessorBase(const TokenProcessorPack<TokenProcessorAlgoT> &processor_pack) : m_processor_pack{processor_pack}
	{}

	/// copy constructor: disabled
	TokenProcessorBase(const TokenProcessorBase&) = delete;

//destructor
	virtual ~TokenProcessorBase() = default;

//overloaded operators
	/// copy assignment operators: disabled
	TokenProcessorBase& operator=(const TokenProcessorBase&) = delete;
	TokenProcessorBase& operator=(const TokenProcessorBase&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<TokenT>) = 0;

	/// get a copy of the processing result
	virtual bool TryGetResult(std::unique_ptr<ResultT>&) = 0;

	/// get notified there won't be any more tokens
	virtual void NotifyNoMoreTokens() = 0;

protected:
//member variables
	TokenProcessorPack<TokenProcessorAlgoT> m_processor_pack{};
};


////
// unimplemented sample interface (this only works when specialized)
// it's best to inherit from the same base for all specializations
///
template <typename TokenProcessorAlgoT>
class TokenProcessor final : public TokenProcessorBase<TokenProcessorAlgoT>
{
public:
//member types
	using TokenT = typename TokenProcessorBase<TokenProcessorAlgoT>::TokenT;
	using ResultT = typename TokenProcessorBase<TokenProcessorAlgoT>::ResultT;

//constructors
	/// default constructor: disabled
	TokenProcessor() = delete;

	/// normal constructor
	TokenProcessor(const TokenProcessorPack<TokenProcessorAlgoT> &processor_pack) : TokenProcessorBase<TokenProcessorAlgoT>{processor_pack}
	{
		assert(false && "TokenProcessor: tried to instantiate class with default template. It must be specialized!");
	}

	/// copy constructor: disabled
	TokenProcessor(const TokenProcessor&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	TokenProcessor& operator=(const TokenProcessor&) = delete;
	TokenProcessor& operator=(const TokenProcessor&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<TokenT>) override {};

	/// try to get a processing result
	virtual bool TryGetResult(std::unique_ptr<ResultT>&) override {return false;};

	/// get notified there won't be more tokens
	virtual void NotifyNoMoreTokens() override {};

private:
//member variables
};




#endif //header guard



















