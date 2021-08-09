// helpers for object tracking

//local headers
#include "assign_objects_algo.h"
#include "async_token_batch_generator.h"
#include "async_token_process.h"
#include "cv_vid_bg_helpers.h"
#include "cv_vid_objecttrack_helpers.h"
#include "cv_vid_frames_generator_algo.h"
#include "exception_assert.h"
#include "highlight_objects_algo.h"
#include "main.h"
#include "mat_set_intermediary.h"
#include "py_dict_consumer.h"

//third party headers
#include <pybind11/pybind11.h>
#include <opencv2/opencv.hpp>   //for video manipulation (mainly)

//standard headers
#include <future>
#include <iostream>
#include <memory>
#include <vector>


/// encapsulates call to async tokenized object tracking analysis
std::unique_ptr<py::dict> TrackObjectsProcess(cv::VideoCapture &vid,
    const VidObjectTrackPack &track_objects_pack,
    std::vector<TokenProcessorPack<HighlightObjectsAlgo>> &highlight_objects_packs,
    std::vector<TokenProcessorPack<AssignObjectsAlgo>> &assign_objects_packs)
{
    // we must have the gil so resource cleanup does not cause segfaults
    //TODO: figure out how to release gil here
    //  (segfault occurs at return of AsyncTokenProcess<AssignObjectsAlgo, PyDictConsumer::final_result_type>::Run())
    py::gil_scoped_acquire gil;

    // expect only one assignbobjects_pack
    assert(assign_objects_packs.size() == 1);

    // set batch size
    int batch_size{static_cast<int>(highlight_objects_packs.size())};
    assert(batch_size);

    // create crop-window for processing frames
    cv::Rect frame_dimensions{GetCroppedFrameDims(track_objects_pack.crop_x, track_objects_pack.crop_y, track_objects_pack.crop_width, track_objects_pack.crop_height,
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)))};

    /// create frame generator

    // cap range of frames to analyze at the frame limit
    long long num_frames{};
    if (vid.isOpened())
    {
        num_frames = static_cast<long long>(vid.get(cv::CAP_PROP_FRAME_COUNT));
        
        // if frame limit is <= 0 use default (all frames in vid)
        if (track_objects_pack.frame_limit > 0)
        {
            if (num_frames > track_objects_pack.frame_limit)
                num_frames = track_objects_pack.frame_limit;
        }
    }
    else
        return nullptr;

    // frame generator packs
    std::vector<TokenGeneratorPack<CvVidFramesGeneratorAlgo>> generator_packs{};
    generator_packs.emplace_back(TokenGeneratorPack<CvVidFramesGeneratorAlgo>{
        batch_size,
        batch_size,
        1,  // frames are not chunked
        track_objects_pack.vid_path,
        0,
        num_frames,
        frame_dimensions,
        track_objects_pack.grayscale,
        track_objects_pack.vid_is_grayscale,
        0,
        0
    });

    // frame generator
    auto frame_gen{std::make_shared<AsyncTokenBatchGenerator<CvVidFramesGeneratorAlgo>>(
        batch_size,
        track_objects_pack.print_timing_report,
        track_objects_pack.token_storage_limit
    )};

    frame_gen->StartGenerator(std::move(generator_packs));

    // create mat shuttle that passes frames with highlighted objects to assign objects algo
    auto mat_shuttle{std::make_shared<MatSetIntermediary>(batch_size,
        track_objects_pack.print_timing_report,
        track_objects_pack.token_storage_limit)};

    // create consumer that collects final objects archive
    auto dict_collector{std::make_shared<PyDictConsumer>(1,
        track_objects_pack.print_timing_report)};

    // create process for highlighting objects
    using highlight_objects_proc_t = AsyncTokenProcess<HighlightObjectsAlgo, MatSetIntermediary::final_result_type>;
    highlight_objects_proc_t highlight_objects_proc{
        batch_size,
        true,
        track_objects_pack.print_timing_report,
        track_objects_pack.token_storage_limit,
        track_objects_pack.token_storage_limit,
        frame_gen,
        mat_shuttle};

    // create process for identifying and tracking objects
    AsyncTokenProcess<AssignObjectsAlgo, PyDictConsumer::final_result_type> assign_objects_proc{
        1,
        true,
        track_objects_pack.print_timing_report,
        track_objects_pack.token_storage_limit,
        track_objects_pack.token_storage_limit,
        mat_shuttle,    // note:: token consumer of previous process is generator for this process
        dict_collector};

    // start the highlighting objects process in a new thread
    auto hbproc_handle = std::async(&highlight_objects_proc_t::Run, &highlight_objects_proc, std::move(highlight_objects_packs));

    // run the assign objects process in this thread (should run synchronously)
    auto object_archive{assign_objects_proc.Run(std::move(assign_objects_packs))};

    // close out the highlight objects thread (it should be waiting at this point)
    // note: we don't care about the return value
    hbproc_handle.get();

    // print out timing info
    if (track_objects_pack.print_timing_report)
    {
        std::cout << "Highlight objects timing report:\n";
        std::cout << highlight_objects_proc.GetTimingInfoAndResetTimer();

        std::cout << "Assign objects timing report:\n";
        std::cout << assign_objects_proc.GetTimingInfoAndResetTimer();
    }

    // return the dictionary of all objects tracked
    if (object_archive && object_archive->size())
        return std::move(object_archive->front());
    else
        return nullptr;
}

/// WARNING: can only be called when the python GIL is held
py::dict TrackObjects(const VidObjectTrackPack &track_objects_pack)
{
    // open video file
    cv::VideoCapture vid{track_objects_pack.vid_path};

    if (!vid.isOpened())
    {
        std::cerr << "Video file not detected: " << track_objects_pack.vid_path << '\n';

        return py::dict{};
    }

    // validate inputs
    // frame cropping should match background image passed in
    EXCEPTION_ASSERT(track_objects_pack.highlight_objects_pack.background.data && !track_objects_pack.highlight_objects_pack.background.empty());
    cv::Rect temp_frame_dimensions{GetCroppedFrameDims(track_objects_pack.crop_x, track_objects_pack.crop_y, track_objects_pack.crop_width, track_objects_pack.crop_height,
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(vid.get(cv::CAP_PROP_FRAME_HEIGHT)))};
    EXCEPTION_ASSERT(temp_frame_dimensions.width == track_objects_pack.highlight_objects_pack.background.cols);
    EXCEPTION_ASSERT(temp_frame_dimensions.height == track_objects_pack.highlight_objects_pack.background.rows);

    // the structuring element should exist
    EXCEPTION_ASSERT(track_objects_pack.highlight_objects_pack.struct_element.data && !track_objects_pack.highlight_objects_pack.struct_element.empty());

    /// create algo packs

    // get number of frames to highlight in parallel based on number of threads available
    // 3 -> min threads required; 0 -> add no extra threads (heuristic from testing)
    // + 1 -> roll one of the required threads into the additional threads obtained to get the batch size
    int batch_size{GetAdditionalThreads(3, 0, track_objects_pack.max_threads) + 1};

    // highlight objects algo packs
    std::vector<TokenProcessorPack<HighlightObjectsAlgo>> highlight_objects_packs{};
    highlight_objects_packs.reserve(batch_size);
    for (int i{0}; i < batch_size; i++)
    {
        // all packs are copies of the input pack
        highlight_objects_packs.emplace_back(track_objects_pack.highlight_objects_pack);

        // must manually clone the cv::Mats here because their copy constructor only creates a reference
        highlight_objects_packs[i].background = track_objects_pack.highlight_objects_pack.background.clone();
        highlight_objects_packs[i].struct_element = track_objects_pack.highlight_objects_pack.struct_element.clone();
    }

    // there is only one assign objects pack
    std::vector<TokenProcessorPack<AssignObjectsAlgo>> assign_objects_packs{};
    assign_objects_packs.emplace_back(track_objects_pack.assign_objects_pack);

    // call the process
    std::unique_ptr<py::dict> objects_archive{
        TrackObjectsProcess(vid, track_objects_pack, highlight_objects_packs, assign_objects_packs)};

    // return the dictionary of tracked objects
    if (objects_archive)
        return std::move(*(objects_archive.release()));
    else
        return py::dict{};
}



















