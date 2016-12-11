#pragma once

#include <map>
#include <set>

#include "settings.hpp"

/*
    Programm option class.
*/
class Options
{
public:
    Options() {};
    void parse(int count, const char *const *args);
    void dump() const noexcept;

    template <typename T>
    void add(const std::string &lk, const std::string &k, const std::string &desc, T default_value);

    template <typename T>
    T get(const std::string &k) const;

private:
    std::map<std::string, SettingItem>::const_iterator iterator(const std::string &k) const throw (std::exception);
    std::string usage(const std::string &program) const;

private:
    std::set<std::string> m_LongKeys;
    std::map<std::string, SettingItem> m_Items;
};
