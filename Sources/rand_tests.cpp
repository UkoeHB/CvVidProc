// random tests

//paired header
#include "rand_tests.h"

//local headers
#include "assign_objects_algo.h"
#include "cv_vid_bg_helpers.h"
#include "cv_vid_objecttrack_helpers.h"
#include "exception_assert.h"
#include "highlight_objects_algo.h"
#include "main.h"
#include "ndarray_converter.h"
#include "project_config.h"
#include "string_utils.h"
#include "ts_interval_timer.h"

//third party headers
#include <opencv2/opencv.hpp>   //for video manipulation (mainly)
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

//standard headers
#include <cassert>
#include <iostream>
#include <memory>


namespace py = pybind11;
using namespace py::literals;

namespace rand_tests
{

// test object highlighting
void test_bubblehighlighting(cv::Mat &background_frame, const CommandLinePack &cl_pack, bool add_test_objecttracking)
{
    // for test video 'bubble_15000fps.mp4'

    // set parameters
    TokenProcessorPack<HighlightObjectsAlgo> objects_pack{
        background_frame,
        cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, cv::Size{4, 4}),
        14,
        7,
        16,
        20,
        20,
        5
    };

    // create algo object for processing frames (highlighting bubbles)
    HighlightObjectsAlgo highlight_objects{objects_pack};

    // get 10th frame of video
    cv::VideoCapture vid{cl_pack.vid_path};
    vid.set(cv::CAP_PROP_POS_FRAMES, 10);

    cv::Mat vid_frame{};
    vid >> vid_frame;

    // grayscale (if vid is grayscale)
    cv::Mat modified_frame{};
    cv::extractChannel(vid_frame, modified_frame, 0);

    // convert format
    std::unique_ptr<cv::Mat> frame{std::make_unique<cv::Mat>(modified_frame)};

    // insert to algo (processes the frame)
    highlight_objects.Insert(std::move(frame));

    // get the processed result out
    frame = highlight_objects.TryGetResult();

    // display the highlighted bubbles
    if (frame && frame->data && !frame->empty())
    {
        cv::imshow("Highlighted Bubbles", *frame);

        // wait for a keypress before ending
        int keypress{cv::waitKey()};
    }
    else
        std::cerr << "Bubbles frame created was malformed, unexpectedly!\n";

    if (add_test_objecttracking)
    {
        test_assignobjects(*frame);
    }
}

void test_embedded_python()
{
    // test python interpreter
    // create Python interpreter
    std::cout << "Starting Python interpreter...\n";
    py::scoped_interpreter guard{};

    // add location of local python libs to path so they can be found
    py::module_ sys = py::module_::import("sys");
    py::object path = sys.attr("path");
    path.attr("insert")(0, config::pylibs_dir);

    py::module_ test = py::module_::import("test1");
    py::function testfunc = test.attr("testfunc");

    // pass dictionary to function that mutates it to see if mutations are saved
    py::dict mydict;
    testfunc(mydict, "key", 22);
    testfunc(mydict, "key", 13);

    // reset dictionary
    mydict = py::dict{};
    testfunc(mydict, "key2", 8);

    // try to put the dictionary in a shared pointer
    std::unique_ptr<py::dict> temp_ptr{std::make_unique<py::dict>(std::move(mydict))};
    testfunc(*temp_ptr, "key3", 99);
    py::dict newdict{std::move(*(temp_ptr.release()))};
    testfunc(newdict, "key4", 1000);
    bool move_succeeded{false};
    try { testfunc(mydict, "key5", 123456); } catch (...) { move_succeeded = true; }
    assert(move_succeeded);

    // can recreate the dictionary after move
    mydict = py::dict{};
    testfunc(mydict, "key6", 654321);

    // test custom converter
    // set up the Numpy array <-> cv::Mat converter
    // courtesy of https://github.com/edmBernard/pybind11_opencv_numpy
    NDArrayConverter::init_numpy();

    cv::Mat testmat{5,10, CV_8UC1};
    testfunc(mydict, "mat", testmat);
}

/// note: assumes test_frame contains highlighted bubbles
/// this test is just to see if the AssignObjectsAlgo works with one input frame
void test_assignobjects(cv::Mat &test_frame)
{
    // create Python interpreter
    std::cout << "Starting Python interpreter...\n";
    py::scoped_interpreter guard{};

    // add location of bubbletracking python libs to path so they can be found
    py::module_ sys = py::module_::import("sys");
    py::object path = sys.attr("path");
    std::string lib_dir{config::bubbletracking_dir};
    lib_dir += "/src/";
    path.attr("insert")(0, lib_dir.c_str());

    // get the function to track bubbles with as functor
    py::module_ objecttracking_mod = py::module_::import("cvimproc.improc");
    py::function assignobjects_func = objecttracking_mod.attr("assign_bubbles_cvvidproc");

    // set parameters
    /*
    py::tuple flow_dir{};
    const int fps{};
    const int pix_per_um{};
    const int width_border{};
    const double v_max{};
    const int min_size_reg{};
    */
    std::vector<float> flow_dir_vec{0.05, 1.0};
    py::array flow_dir{static_cast<py::ssize_t>(flow_dir_vec.size()), flow_dir_vec.data()};

    using namespace pybind11::literals;     // for '_a'
    TokenProcessorPack<AssignObjectsAlgo> assignobjects_pack{
        assignobjects_func,
        py::dict{"flow_dir"_a=flow_dir,     // +x direction (?)
        "fps"_a=3,      // not useful here...?
        "pix_per_um"_a=4,
        "width_border"_a=5,
        "row_lo"_a=0,
        "row_hi"_a=test_frame.rows,
        "v_max"_a=200,
        "min_size_reg"_a=40}
    };

    // create new scope so the GIL can be released
    std::unique_ptr<py::dict> objects_archive{nullptr};
    {
        // release GIL (will be acquired by AssignObjectsAlgo)
        py::gil_scoped_release nogil;

        // create algo object for processing frames (highlighting bubbles)
        AssignObjectsAlgo assign_objects{assignobjects_pack};

        // convert format
        std::vector<cv::Mat> vec_temp;
        vec_temp.emplace_back(test_frame.clone());
        std::unique_ptr<std::vector<cv::Mat>> highlighted_objects{std::make_unique<std::vector<cv::Mat>>(std::move(vec_temp))};

        // insert to algo (processes the frame)
        assign_objects.Insert(std::move(highlighted_objects));

        // no more frames to insert (tells the algo to finalize a result)
        assign_objects.NotifyNoMoreTokens();

        objects_archive = assign_objects.TryGetResult();
    }

    // print result
    if (objects_archive)
        py::print(*objects_archive);
    else
        py::print("assign objects returned no archive");
}

//TODO: it's not clear if this is working as intended... need to test with a sample object simpler than AssignObjectsAlgo
void test_timing_numpyconverter(const int num_rounds, const bool include_conversion)
{
    // test python interpreter
    // create Python interpreter
    std::cout << "Starting Python interpreter...\n";
    py::scoped_interpreter guard{};

    // add location of local python libs to path so they can be found
    py::module_ sys = py::module_::import("sys");
    py::object path = sys.attr("path");
    path.attr("insert")(0, config::pylibs_dir);

    // get spinfunc for testing conversion
    py::module_ test = py::module_::import("test1");
    py::function spinfunc = test.attr("spinfunc");

    cv::Mat testmat{5,10, CV_8UC1};
    int num{0};

    // time the init numpy function
    TSIntervalTimer timer{};
    auto start_time{timer.GetTime()};

    for (int round{0}; round < num_rounds; round++)
    {
        // test custom converter
        // set up the Numpy array <-> cv::Mat converter
        // courtesy of https://github.com/edmBernard/pybind11_opencv_numpy
        NDArrayConverter::init_numpy();

        if (include_conversion)
            num = spinfunc(testmat, num).cast<int>();

        // end the timer and print results
        start_time = timer.AddInterval(start_time);
    }

    // print timer report
    auto timer_report{timer.GetReport<std::chrono::milliseconds>()};
    auto interval_ms{timer_report.total_time.count()};
    auto interval_s_float{static_cast<double>(interval_ms/1000.0)};
    std::cout << "init_numpy timing: " << interval_s_float << " s; " <<
        timer_report.num_intervals << " rounds; " <<
        (timer_report.total_time / timer_report.num_intervals).count() << " ms avg\n";
}

/// test exception assert
void test_exception_assert()
{
    // first exercise the string utils

    // format_string()
    const char *buf = "char(%c), int(%i), float(%f), cstring(%s)";
    std::cout << format_string(buf, 'x', 5, 2.2, "hello world") << '\n';

    // then test exception asserts

    // assert(false) should throw an exception
    try
    {
        EXCEPTION_ASSERT(false);
        std::cout << "EXCEPTION_ASSERT(false) test failed!\n";
    }
    catch (std::exception &exc)
    {
        std::cout << "EXCEPTION_ASSERT(false) test succeeded! Output: \n" << exc.what() << '\n';
    }
        
    // assert(true) should not throw an exception
    try
    {
        EXCEPTION_ASSERT(true);
        std::cout << "EXCEPTION_ASSERT(true) test succeeded!\n";
    }
    catch (std::exception &exc)
    {
        std::cout << "EXCEPTION_ASSERT(true) test failed! Output: \n" << exc.what() << '\n';
    }

    // assert(expression) should propagate the exception correctly
    try
    {
        EXCEPTION_ASSERT(10 == 11);
    }
    catch (std::exception &exc)
    {
        std::cout << "EXCEPTION_ASSERT(10 == 11) test! Output: \n" << exc.what() << '\n';
    }

    // assert(expression, message) should propagate the message correctly
    try
    {
        EXCEPTION_ASSERT_MSG(false, "exception assert with message - success");
    }
    catch (std::exception &exc)
    {
        std::cout << "EXCEPTION_ASSERT_MSG() test! Output: \n" << exc.what() << '\n';
    }
}

/// demo the TrackObjects function
void demo_trackobjects(CommandLinePack &cl_pack, cv::Mat &background_frame)
{
    // create Python interpreter
    std::cout << "Starting Python interpreter...\n";
    py::scoped_interpreter guard{};

    // add location of bubbletracking python libs to path so they can be found
    py::module_ sys = py::module_::import("sys");
    py::object path = sys.attr("path");
    std::string lib_dir{config::bubbletracking_dir};
    lib_dir += "/src/";
    path.attr("insert")(0, lib_dir.c_str());

    // get the function to track bubbles with as functor
    py::module_ objecttracking_mod = py::module_::import("cvimproc.improc");
    py::function assignobjects_func = objecttracking_mod.attr("assign_bubbles_cvvidproc");

    // create template highlightbubbles pack
    TokenProcessorPack<HighlightObjectsAlgo> highlightobjects_pack{
        background_frame.clone(),
        cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, cv::Size{4, 4}),
        14,
        7,
        16,
        20,
        20,
        5
    };

    // create template assignbubbles pack
    std::vector<float> flow_dir_vec{0.05, 1.0};
    py::array flow_dir{static_cast<py::ssize_t>(flow_dir_vec.size()), flow_dir_vec.data()};

    using namespace pybind11::literals;     // for '_a'
    TokenProcessorPack<AssignObjectsAlgo> assignobjects_pack{
        assignobjects_func,
        py::dict{"flow_dir"_a=flow_dir,     // +x direction (?)
        "fps"_a=3,      // not useful here...?
        "pix_per_um"_a=4,
        "width_border"_a=5,
        "row_lo"_a=0,
        "row_hi"_a=background_frame.rows,
        "v_max"_a=200,
        "min_size_reg"_a=40}
    };

    // create parameter pack
    VidObjectTrackPack trackbubble_pack{cl_pack.vid_path,
        highlightobjects_pack,
        assignobjects_pack,
        cl_pack.max_threads,
        cl_pack.bg_frame_lim,
        cl_pack.grayscale,
        cl_pack.vid_is_grayscale,
        0,
        0,
        0,
        0,
        10,
        cl_pack.print_timing_report
    };

    /// track bubbles in the input video

    // start timer
    TSIntervalTimer timer{};
    auto start_time{timer.GetTime()};

    // run the process
    std::cout << "\nTracking bubbles...\n";
    py::dict objects_archive{TrackObjects(trackbubble_pack)};

    // end the timer and print results
    timer.AddInterval(start_time);
    auto timer_report{timer.GetReport<std::chrono::milliseconds>()};
    auto interval_ms{timer_report.total_time.count()};
    auto interval_s_float{static_cast<double>(interval_ms/1000.0)};
    std::cout << "Bubbles tracked in: " << interval_s_float << " seconds\n";

    // print info about results
    if (static_cast<bool>(objects_archive))
        std::cout << "Number of bubbles: " << objects_archive.size() << '\n';
    else
        std::cout << "No bubbles tracked!\n";
}


} //rand_tests namespace












