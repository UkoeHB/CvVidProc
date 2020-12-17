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

/// expected interface of TokenProcessor specializations
template <typename TokenProcessorT, typename TokenT>
class TokenProcessorBase
{
public:
//constructors
	/// default constructor: disabled
	TokenProcessorBase() = delete;

	/// normal constructor
	TokenProcessorBase(const TokenProcessorPack<TokenProcessorT> &processor_pack) : m_processor_pack{processor_pack}
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
	virtual TokenT GetResult() const = 0;

protected:
//member variables
	TokenProcessorPack<TokenProcessorT> m_processor_pack{};
};


////
// unimplemented sample interface (this only works when specialized)
// it's best to inherit from the same base for all specializations
///
template <typename TokenProcessorT, typename TokenT>
class TokenProcessor final : public TokenProcessorBase<TokenProcessorT, TokenT>
{
public:
//constructors
	/// default constructor: disabled
	TokenProcessor() = delete;

	/// normal constructor
	TokenProcessor(const TokenProcessorPack<TokenProcessorT> &processor_pack) : TokenProcessorBase<TokenProcessorT, TokenT>{processor_pack}
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

	/// get the processing result
	virtual TokenT GetResult() const override {return TokenT{};};

private:
//member variables
};




#endif //header guard



















