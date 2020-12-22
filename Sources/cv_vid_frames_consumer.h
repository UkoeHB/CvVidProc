


template <typename FrameProcessorAlgoT, typename FinalResultT>
class CvVidFramesConsumer : public AsyncTokenProcess<FrameProcessorAlgoT, FinalResultT>
{
	static_assert(std::is_same<FrameProcessorAlgoT::token_type, cv::Mat>::value,
			"Frame processor implementation does not have token type cv::Mat!");
};

//generates frames
//converts frames to batches (frame segments = tokens in batch)
//specified algo is used to consume segments
//unspecified: how results are handled (must have derived class)


template <typename FrameProcessorAlgoT>
class CvVidBackground final : public CvVidFramesConsumer<FrameProcessorAlgoT, cv::Mat>
{


};

//ignores all except very last result from all token processing units (unless specialized)










