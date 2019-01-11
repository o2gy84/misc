#include <iostream>
#include <chrono>
#include <vector>

#include <xmmintrin.h>
#include <math.h>


namespace
{

float l2sqr_dist_sse2(const float *pVect1, const float *pVect2, std::size_t qty)
// sqrt of Euclid distance
{
    static_assert(sizeof(float) == 4, "Cannot use SIMD instructions with non-32-bit floats.");

    std::size_t qty4  = qty/4;
    std::size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2;
    __m128  sum = _mm_set1_ps(0);

    while (pVect1 < pEnd1)
    {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    while (pVect1 < pEnd2)
    {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3)
    {
        float diff2 = *pVect1++ - *pVect2++;
        res += diff2 * diff2;
    }

    return res;
}

float euclidean_distance_impl(const std::vector<float> &v1, const std::vector<float> &v2)
//    Pseudocode:
//    float dist = 0;
//    for (size_t i = 0; i < v1.size(); ++i)
//    {
//        float v = (v1[i] - v2[i]);
//        v = v * v;
//        dist += v;
//    }
//    return sqrt(dist);
{
    if (v1.size() != v2.size())
    {
        throw std::runtime_error("bad vectors");
    }

    float s = l2sqr_dist_sse2(v1.data(), v2.data(), v1.size());
    return sqrt(s);
}

}   // namespace


float euclidean_distance(const std::vector<float> &v1, const std::vector<float> &v2)
{
    return euclidean_distance_impl(v1, v2);
}

float dot_product(const std::vector<float> &v1, const std::vector<float> &v2)
{
    if (v1.size() != v2.size())
    {
        throw std::runtime_error("bad vectors");
    }

    float res = 0;
    for (int i = 0; i < v1.size(); ++i)
    {
        res += v1[i]*v2[i];
    }

    return res;
}

float global_sum = 0;               // need common sum to prevent compiler optimizations
std::vector<float> v1(128, 0.0);
std::vector<float> v2(128, 0.0);

void test_impl(int n)
{
    for (float &f : v1)
    {
        f += n;
    }
    for (float &f : v2)
    {
        f -= n;
    }

    global_sum += euclidean_distance(v1, v2);
    global_sum += dot_product(v1, v2);
}

int test(size_t num)
{
    std::cerr << "iterations: " << num << "\n";

    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < num; ++i)
    {
        test_impl(i);
    }

    auto end = std::chrono::steady_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cerr << "total: " << ms << " ms\n";
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: ./" << argv[0] << " num\n";
        exit(-1);
    }

    try
    {
        test(std::stoul(argv[1]));
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
        exit(-1);
    }

    std::cerr << "sum: " << global_sum << "\n";
    return 0;
}
