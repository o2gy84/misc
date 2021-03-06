#include <vector>
#include <string>

namespace utils
{
    std::vector<std::string> split(const std::string &line, const std::string &delimiter);

    bool starts_with(const std::string &s, const std::string &predicate);
}
