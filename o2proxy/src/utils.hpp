#include <vector>
#include <string>

namespace utils
{
    std::string int2ipv4(uint32_t ip);

    std::vector<std::string> split(const std::string &line, const std::string &delimiter);
    std::vector<std::string> split(const std::string &line, const std::string &delimiter, int max_parts);

    bool starts_with(const std::string &s, const std::string &predicate);
    bool ends_with(const std::string &s, const std::string &predicate);

    std::string lowercased(const std::string &str);
    std::string trimmed(std::string s);

}
