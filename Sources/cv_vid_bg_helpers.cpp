// helpers for getting background of an opencv vid

//paired header
#include "cv_vid_bg_helpers.h"

//local headers
#include "async_token_batch_generator.h"
#include "async_token_process.h"
#include "cv_vid_frames_generator_algo.h"
#include "cv_vid_fragment_consumer.h"
#include "exception_assert.h"
#include "histogram_median_algo.h"
#include "main.h"

//third party headers
#include <opencv2/opencv.hpp>   //for video manipulation (mainly)

//standard headers
#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>


/// get background algo from string
BGAlgo GetBGAlgo(const std::string &algo)
{
    if (algo == "hist")
        return BGAlgo::HISTOGRAM;
    else
    {
        std::cerr << "Unknown background algorithm detected: " << algo << '\n';
        return BGAlgo::UNKNOWN;
    }
}

cv::Rect GetCroppedFrameDims(int x, int y, int width, int height, int hor_pixels, int vert_pixels)
{
    // note: the returned frame is not allowed to be empty
    EXCEPTION_ASSERT(x >= 0);
    EXCEPTION_ASSERT(y >= 0);
    EXCEPTION_ASSERT(width >= 0);
    EXCEPTION_ASSERT(height >= 0);
    EXCEPTION_ASSERT_MSG(hor_pixels > 0, "frame must have horizontal size");
    EXCEPTION_ASSERT_MSG(vert_pixels > 0, "frame must have verical size");
    EXCEPTION_ASSERT_MSG(x < hor_pixels, "start of crop window can't be outside frame");
    EXCEPTION_ASSERT_MSG(y < vert_pixels, "start of crop window can't be outside frame");

    // width can be no greater than nominal width of frame
    if (width == 0 || width + x > hor_pixels)
        width = hor_pixels - x;

    // height can be no greater than nominal height of frame
    if (height == 0 || height + y > hor_pixels)
        height = vert_pixels - y;

    return cv::Rect{x, y, width, height};
}

template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgo(cv::VideoCapture &vid,
    const VidBgPack &vidbg_pack,
    std::vector<TokenProcessorPack<MedianAlgo>> &processor_packs,
    const int generator_threads,
    const bool synchronous_allowed)
{
    // number of fragments to create during background analysis
    int batch_size{static_cast<int>(processor_packs.size())};
    assert(batch_size);

    cv::Rect frame_dimensions{GetCroppedFrameDims(vidbg_pack.crop_x, vidbg_pack.crop_y, vidbg_pack.crop_width, vidbg_pack.crop_height,
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)))};

    /// create frame generator

    // frame generator packs
    std::vector<TokenGeneratorPack<CvVidFramesGeneratorAlgo>> generator_packs{};
    assert(generator_threads >= 1);
    generator_packs.reserve(generator_threads);

    // divide the video into ranges of frames for the async generator
    long long begin_frame{0};
    long long sum_frame{0};
    long long remainder_frames{0};
    if (vid.isOpened())
    {
        int num_frames{static_cast<int>(vid.get(cv::CAP_PROP_FRAME_COUNT))};

        // cap range of frames to analyze at the frame limit
        if (vidbg_pack.frame_limit > 0)
        {
            if (num_frames > vidbg_pack.frame_limit)
                num_frames = vidbg_pack.frame_limit;
        }

        sum_frame = num_frames / generator_threads;
        remainder_frames = num_frames % generator_threads;
    }

    for (std::size_t i{0}; i < generator_threads; i++)
    {
        generator_packs.emplace_back(TokenGeneratorPack<CvVidFramesGeneratorAlgo>{
            batch_size,
            1,
            batch_size,
            vidbg_pack.vid_path,
            begin_frame,
            begin_frame + sum_frame + (i + 1 == generator_threads ? remainder_frames : 0),
            frame_dimensions,
            vidbg_pack.grayscale,
            vidbg_pack.vid_is_grayscale,
            0,  //no buffer
            0   //no buffer
        });

        begin_frame += sum_frame;
    }

    // frame generator
    auto frame_gen{std::make_shared<AsyncTokenBatchGenerator<CvVidFramesGeneratorAlgo>>(
        batch_size,
        vidbg_pack.print_timing_report,
        vidbg_pack.token_storage_limit
    )};

    frame_gen->StartGenerator(std::move(generator_packs));

    // create fragment consumer
    auto bg_frag_consumer{std::make_shared<CvVidFragmentConsumer>(batch_size,
        vidbg_pack.print_timing_report,
        0,  //no buffer
        0,  //no buffer
        frame_dimensions.width,
        frame_dimensions.height
    )};

    // create process
    AsyncTokenProcess<MedianAlgo, CvVidFragmentConsumer::final_result_type> vid_bg_prod{batch_size,
        synchronous_allowed,
        vidbg_pack.print_timing_report,
        vidbg_pack.token_storage_limit,
        vidbg_pack.token_storage_limit,
        frame_gen,
        bg_frag_consumer
    };

    // run process to get background image
    auto bg_img{vid_bg_prod.Run(std::move(processor_packs))};

    // print out timing info
    if (vidbg_pack.print_timing_report)
        std::cout << vid_bg_prod.GetTimingInfoAndResetTimer();

    if (bg_img && !bg_img->empty())
        return std::move(bg_img->back());
    else
        return cv::Mat{};
}

template <typename MedianAlgo>
cv::Mat VidBackgroundWithAlgoEmptyPacks(cv::VideoCapture &vid, const VidBgPack &vidbg_pack)
{
    // set the batch size
    int batch_size{GetAdditionalThreads(1, 0, vidbg_pack.max_threads)};
    bool synchronous{false};

    // if no batch size specified then use synchronous mode (should fall through downstream)
    // should only happen if user specified max_threads=1 or the hardware concurrency is unavailable
    int generator_threads{1};

    if (batch_size <= 0)
    {
        batch_size = 1;
        synchronous = true;
    }
    else
    {
        // divide available threads between the token generator and processor: HEURISTIC
        int total_threads = generator_threads + batch_size;
        generator_threads = total_threads / 2;
        batch_size = total_threads - generator_threads;     //processor gets extra thread in case of odd total number
    }

    assert(generator_threads);
    assert(batch_size);

    std::vector<TokenProcessorPack<MedianAlgo>> empty_packs;
    empty_packs.resize(batch_size, TokenProcessorPack<MedianAlgo>{});

    return VidBackgroundWithAlgo<MedianAlgo>(vid, vidbg_pack, empty_packs, generator_threads, synchronous);
}

/// get a video background
cv::Mat GetVideoBackground(const VidBgPack &vidbg_pack)
{
    // open video file
    cv::VideoCapture vid{vidbg_pack.vid_path};

    if (!vid.isOpened())
    {
        std::cerr << "Video file not detected: " << vidbg_pack.vid_path << '\n';

        return cv::Mat{};
    }

    // print info about the video
    long long total_frames{static_cast<long long>(vid.get(cv::CAP_PROP_FRAME_COUNT))};

    std::cout << "Frames: " << total_frames <<
                  "; Res: " << vid.get(cv::CAP_PROP_FRAME_WIDTH) << 'x' << vid.get(cv::CAP_PROP_FRAME_HEIGHT);

    if (vidbg_pack.crop_x || vidbg_pack.crop_y || vidbg_pack.crop_width || vidbg_pack.crop_height)
    {
        cv::Rect frame_dimensions{GetCroppedFrameDims(vidbg_pack.crop_x, vidbg_pack.crop_y, vidbg_pack.crop_width, vidbg_pack.crop_height,
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)))};

        std::cout << "(" << frame_dimensions.width << 'x' << frame_dimensions.height << " cropped)";
    }
    std::cout << "; FPS: " << vid.get(cv::CAP_PROP_FPS) << '\n';

    // figure out how many frames will be analyzed
    long long frames_to_analyze{vidbg_pack.frame_limit};

    if (frames_to_analyze <= 0 || frames_to_analyze > total_frames)
        frames_to_analyze = total_frames;

    // algo is user-specified
    switch (GetBGAlgo(vidbg_pack.bg_algo))
    {
        case BGAlgo::HISTOGRAM :
        {
            // use cheapest histogram algorithm
            if (frames_to_analyze <= static_cast<long long>(static_cast<unsigned char>(-1)))
            {
                return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo8>(vid, vidbg_pack);
            }
            else if (frames_to_analyze <= static_cast<long long>(static_cast<std::uint16_t>(-1)))
            {
                return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo16>(vid, vidbg_pack);
            }
            else if (frames_to_analyze <= static_cast<long long>(static_cast<std::uint32_t>(-1)))
            {
                return VidBackgroundWithAlgoEmptyPacks<HistogramMedianAlgo32>(vid, vidbg_pack);
            }
            else
            {
                std::cerr << "warning, video appears to have over 2^32 frames! (" << total_frames << ") is way too many!\n";
            }
        }

        default :
        {
            std::cerr << "tried to get vid background with unknown algorithm: " << vidbg_pack.bg_algo << '\n';

            return cv::Mat{};
        }
    };

    return cv::Mat{};
}




















