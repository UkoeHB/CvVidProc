// background processor template for managing an image background processing algorithm

#ifndef BACKGROUND_PROCESSOR_039587849_H
#define BACKGROUND_PROCESSOR_039587849_H

//local headers

//third party headers

//standard headers
#include <cassert>
#include <memory>


/// by default, the processor pack is empty; add specializations for specific content
template<typename T>
struct BgProcessorPack final
{};

/// expected interface of BackgroundProcessor specializations
template<typename BgProcessorT, typename MaterialT>
class BackgroundProcessorBase
{
public:
//constructors
	/// default constructor: disabled
	BackgroundProcessorBase() = delete;

	/// normal constructor
	BackgroundProcessorBase(const BgProcessorPack<BgProcessorT> &processor_pack) : m_processor_pack{processor_pack}
	{}

	/// copy constructor: disabled
	BackgroundProcessorBase(const BackgroundProcessorBase&) = delete;

//destructor
	virtual ~BackgroundProcessorBase() = default;

//overloaded operators
	/// copy assignment operators: disabled
	BackgroundProcessorBase& operator=(const BackgroundProcessorBase&) = delete;
	BackgroundProcessorBase& operator=(const BackgroundProcessorBase&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<MaterialT>) = 0;

	/// get a copy of the processing result
	virtual MaterialT GetResult() const = 0;

protected:
//member variables
	BgProcessorPack<BgProcessorT> m_processor_pack{};
};


////
// unimplemented sample interface (this only works when specialized)
// it's best to inherit from the same base for all specializations
///
template<typename BgProcessorT, typename MaterialT>
class BackgroundProcessor final : public BackgroundProcessorBase<BgProcessorT, MaterialT>
{
public:
//constructors
	/// default constructor: disabled
	BackgroundProcessor() = delete;

	/// normal constructor
	BackgroundProcessor(const BgProcessorPack<BgProcessorT> &processor_pack) : BackgroundProcessorBase<BgProcessorT, MaterialT>{processor_pack}
	{
		assert(false && "BackgroundProcessor: tried to instantiate class with default template. It must be specialized!");
	}

	/// copy constructor: disabled
	BackgroundProcessor(const BackgroundProcessor&) = delete;

//destructor: not needed (final class)

//overloaded operators
	/// copy assignment operators: disabled
	BackgroundProcessor& operator=(const BackgroundProcessor&) = delete;
	BackgroundProcessor& operator=(const BackgroundProcessor&) const = delete;

//member functions
	/// insert an element to be processed
	virtual void Insert(std::unique_ptr<MaterialT>) override {};

	/// get the processing result
	virtual MaterialT GetResult() const override {return MaterialT{};};

private:
//member variables
};




#endif //header guard



















