// highlights objects in an image

#ifndef HIGHLIGHT_OBJECTS_ALGO_6787878_H
#define HIGHLIGHT_OBJECTS_ALGO_6787878_H

//local headers
#include "token_processor_algo.h"

//third party headers
#include <opencv2/opencv.hpp>

//standard headers
#include <memory>
#include <vector>


/// processor algorithm type declaration
class HighlightObjectsAlgo;

/// specialization of variable pack
template <>
struct TokenProcessorPack<HighlightObjectsAlgo> final
{
    cv::Mat background{};
    cv::Mat struct_element{};
    const int threshold{};
    const int threshold_lo{};
    const int threshold_hi{};
    const int min_size_hyst{};
    const int min_size_threshold{};
    const int width_border{};
};

////
// uses heuristics to process a cv::Mat image so objects are highlighted
///
class HighlightObjectsAlgo final : public TokenProcessorAlgo<HighlightObjectsAlgo, cv::Mat, cv::Mat>
{
public:
//constructors
    /// default constructor: disabled
    HighlightObjectsAlgo() = delete;

    /// normal constructor
    HighlightObjectsAlgo(TokenProcessorPack<HighlightObjectsAlgo> processor_pack) : TokenProcessorAlgo{std::move(processor_pack)}
    {}

    /// copy constructor: disabled
    HighlightObjectsAlgo(const HighlightObjectsAlgo&) = delete;

//destructor: not needed (final class)

//overloaded operators
    /// copy assignment operators: disabled
    HighlightObjectsAlgo& operator=(const HighlightObjectsAlgo&) = delete;
    HighlightObjectsAlgo& operator=(const HighlightObjectsAlgo&) const = delete;

//member functions
    /// insert an element to be processed
    /// note: overrides result if there already is one
    virtual void Insert(std::unique_ptr<cv::Mat> in_mat) override
    {
        // leave if image not available
        if (!in_mat || !in_mat->data || in_mat->empty())
            return;

        HighlightObjects(*in_mat);

        m_result = std::move(in_mat);
    }

    /// get the processing result
    virtual std::unique_ptr<cv::Mat> TryGetResult() override
    {
        // get result if there is one
        if (m_result)
            return std::move(m_result);
        else
            return nullptr;
    }

    /// get notified there are no more elements
    virtual void NotifyNoMoreTokens() override
    {
        // do nothing (each token processed is independent of the previous)
    }

    /// report if there is a result to get
    virtual bool HasResults() override
    {
        return static_cast<bool>(m_result);
    }

    /// C++ implementation of highlight_bubbles()
    void HighlightObjects(cv::Mat &frame);

    /// C++ implementation of thresh_im()
    cv::Mat ThresholdImage(cv::Mat &image, const int threshold);

    /// C++ implementation of hysteresis_threshold()
    cv::Mat ThresholdImageWithHysteresis(cv::Mat &image, const int threshold_lo, const int threshold_hi);

    /// C++ implementation of remove_small_objects_find()
    void RemoveSmallObjects(cv::Mat &image, const int min_size_threshold);

    /// C++ implementation of fill_holes()
    void FillHoles(cv::Mat &image);

    /// C++ implementation of frame_and_fill()
    void FrameAndFill(cv::Mat &image, const int width_border);


private:
//member variables
    /// store result in anticipation of future requests
    std::unique_ptr<cv::Mat> m_result{};
};


#endif //header guard





















