// random tests

#ifndef RAND_TESTS_78989_H
#define RAND_TESTS_78989_H

//local headers
#include "main.h"

//third party headers

//standard headers

//forward declarations
namespace cv {class Mat;}


namespace rand_tests
{

void test_embedded_python();

void test_objecthighlighting(cv::Mat &background_frame, const CommandLinePack &cl_pack, bool add_test_objecttracking);
void test_assignobjects(cv::Mat &test_frame);

void test_timing_numpyconverter(const int num_rounds, const bool include_conversion = false);

void test_exception_assert();

void demo_trackobjects(CommandLinePack &cl_pack, cv::Mat &background_frame);

}


#endif //header guard
