// helpers for object tracking

#ifndef CV_VID_OBJECTTRACK_HELPERS_4738498_H
#define CV_VID_OBJECTTRACK_HELPERS_4738498_H

//local headers
#include "assign_objects_algo.h"
#include "highlight_objects_algo.h"

//third party headers
#include <pybind11/pybind11.h>

//standard headers
#include <memory>
#include <string>
#include <vector>

//forward declarations
namespace cv {class VideoCapture;}


/// settings necessary to track objects in a video
struct __attribute__ ((visibility("hidden"))) VidObjectTrackPack
{
    // path to video
    const std::string vid_path{};

    // pack of variables for HighlightObjectsAlgo
    TokenProcessorPack<HighlightObjectsAlgo> highlight_objects_pack;
    // pack of variables for AssignObjectsAlgo
    TokenProcessorPack<AssignObjectsAlgo> assign_objects_pack;

    // max number of threads allowed
    const int max_threads{-1};

    // max number of frames to analyze (<= 0 means use all frames in video)
    const long long frame_limit{-1};
    // whether to convert frames to grayscale before analyzing them
    const bool grayscale{false};
    // whether video is already supposedly grayscale
    const bool vid_is_grayscale{false};

    // x-position of frame-crop region
    const int crop_x{0};
    // y-position of frame-crop region
    const int crop_y{0};
    // width of frame-crop region
    const int crop_width{0};
    // height of frame-crop region
    const int crop_height{0};

    // max number of input fragments to store at a time (memory conservation vs efficiency)
    const int token_storage_limit{-1};

    // whether to collect and print timing reports
    const bool print_timing_report{false};
};

/// encapsulates call to async tokenized object tracking analysis
std::unique_ptr<py::dict> TrackObjectsProcess(cv::VideoCapture &vid,
    const VidObjectTrackPack &track_objects_pack,
    std::vector<TokenProcessorPack<HighlightObjectsAlgo>> &highlight_objects_packs,
    std::vector<TokenProcessorPack<AssignObjectsAlgo>> &assign_objects_packs);

/// track objects in a video and return record of objects tracked
/// WARNING: can only be called when the python GIL is held
py::dict TrackObjects(const VidObjectTrackPack &trackbubble_pack);


#endif //header guard














