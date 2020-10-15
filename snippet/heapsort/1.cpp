#include <iostream>
#include <string>
#include <algorithm>
#include <vector>


struct Test {
    int zone;
    std::string text;

    bool operator<(const Test &t) const {
        if (zone < t.zone) {
            return true;
        }
        return false;
    }
};

int main(int argc, char *argv[]) {
    std::cerr << "start\n";

    std::vector<Test> vec = {
        {1, "text1"}, {1, "text2"}
    };

    for (size_t i = 0; i < vec.size(); ++i) {
        std::cerr << "addr vec["<<i<<"]=" << (void*)(&vec[i]) << ", text: " << vec[i].text << "\n";
    }

    std::make_heap(vec.begin(), vec.end());

    for (size_t i = 0; i < vec.size(); ++i) {
        std::cerr << "addr vec["<<i<<"]=" << (void*)(&vec[i]) << ", text: " << vec[i].text << "\n";
    }

    std::pop_heap(vec.begin(), vec.end());
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cerr << "addr vec["<<i<<"]=" << (void*)(&vec[i]) << ", text: " << vec[i].text << "\n";
    }

    std::pop_heap(vec.begin(), vec.end());
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cerr << "addr vec["<<i<<"]=" << (void*)(&vec[i]) << ", text: " << vec[i].text << "\n";
    }

    std::pop_heap(vec.begin(), vec.end());
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cerr << "addr vec["<<i<<"]=" << (void*)(&vec[i]) << ", text: " << vec[i].text << "\n";
    }

    return 0;
}
