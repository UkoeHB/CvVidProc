// random tests

#ifndef RAND_TESTS_78989_H
#define RAND_TESTS_78989_H

//local headers
#include "main.h"

//third party headers
#include <opencv2/opencv.hpp>	//for video manipulation (mainly)

//standard headers

namespace rand_tests
{

void test_embedded_python();

void test_bubblehighlighting(cv::Mat &background_frame, const CommandLinePack &cl_pack, bool add_test_bubbletracking);
void test_assignbubbles(cv::Mat &test_frame);

void test_timing_numpyconverter(const int num_rounds, const bool include_conversion = false);

void demo_trackbubbles(CommandLinePack &cl_pack, cv::Mat &background_frame);

}


#endif //header guard