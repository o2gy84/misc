#include <iostream>
#include <algorithm>
#include <string.h>

#include "utils.hpp"

namespace utils
{
    std::string int2ipv4(uint32_t ip)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip&0xFF, (ip&0xFF00) >> 8, (ip&0xFF0000) >> 16, (ip&0xFF000000) >> 24);
        return buf;
    }

    std::vector<std::string> split(const std::string &line, const std::string &delimiter, int max_parts)
    {
        int parts = 0;
        std::vector<std::string> strs;
        size_t prev_pos = 0;
        size_t pos = 0;
        while ((pos = line.find(delimiter, prev_pos)) != std::string::npos)
        {
            ++parts;
            std::string tmp(line.begin() + prev_pos, line.begin() + pos);
            strs.emplace_back(tmp);
            prev_pos = pos + delimiter.size();

            if (parts == max_parts - 1)
                break;
        }

        if (prev_pos + delimiter.size() <= line.size())
        {
            std::string tmp(line.begin() + prev_pos, line.end());
            strs.emplace_back(tmp);
        }

        return strs;

    }

    std::vector<std::string> split(const std::string &line, const std::string &delimiter)
    {
        return split(line, delimiter, -1);
    }

    bool starts_with(const std::string &s, const std::string &predicate)
    {
        if (strncmp(s.data(), predicate.data(), predicate.size()) == 0)
            return true;
        return false;
    }

    bool ends_with(const std::string &s, const std::string &predicate)
    {
        if (s.size() < predicate.size())
            return false;

        if (strncmp(s.data() + s.size() - predicate.size(), predicate.data(), predicate.size()) == 0)
            return true;

        return false;
    }

    std::string lowercased(const std::string &str)
    {
        std::string ret;
        ret.resize(str.size());
        std::transform(str.begin(), str.end(), ret.begin(), ::tolower);
        return ret;
    }

    // trim from start (in place)
    void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    }

    // trim from end (in place)
    void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    }

    // trim from both ends (in place)
    void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    std::string ltrimmed(std::string s)
    {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    std::string rtrimmed(std::string s)
    {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    std::string trimmed(std::string s)
    {
        trim(s);
        return s;
    }
}
