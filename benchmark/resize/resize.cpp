#include <iostream>
#include <memory>
#include <fstream>
#include <map>
#include <chrono>
#include <mutex>

#include "opencv2/highgui/highgui.hpp"  // imdecode, imwrite
#include "opencv2/imgproc/imgproc.hpp"


bool file_exist(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }
    return true;
}

cv::Mat load_image(const std::string &path)
{
    if (!file_exist(path))
    {
        throw std::runtime_error("not exist");
    }

    cv::Mat image = cv::imread(path.c_str(), CV_LOAD_IMAGE_COLOR);
    if (!image.data)
    {
        std::cerr << "opencv failed loading image. image channels: " << image.channels() << "\n";
        throw std::runtime_error(std::string("fail to load image into opencv: ") + path);
    }

    return image;
}

int counter = 0;
std::mutex mutex;

void test_impl(const cv::Mat &input, size_t h, size_t w)
{
    mutex.lock();
    ++counter;
    mutex.unlock();

    cv::Mat output;
    cv::resize(input, output, cv::Size(/*w*/w, /*h*/h));

    cv::Rect roi;
    roi.x = 64;
    roi.y = 64;
    roi.width = 128;
    roi.height = 128;

    output = output(roi);
}

int test(const std::string &path, size_t num)
{
    std::cerr << "iterations: " << num << "\n";

    cv::Mat image = load_image(path);
    std::cerr << "image: w=" << image.size().width << ", h=" << image.size().height << std::endl;

    auto start = std::chrono::steady_clock::now();

#pragma omp parallel
#pragma omp for
    for (size_t i = 0; i < num; ++i)
    {
        test_impl(image, 256, 256);
    }

    auto end = std::chrono::steady_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cerr << "total: " << ms << " ms\n";
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: ./" << argv[0] << " path num\n";
        exit(-1);
    }

    try
    {
        test(argv[1], std::stoul(argv[2]));
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    std::cerr << "counter: " << counter << "\n";
    return 0;
}
