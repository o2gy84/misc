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
    void add(const std::string &lk, const std::string &k, const std::string &desc, T&& default_value)
    {
        if (lk.empty() || k.empty())
        {
            throw std::runtime_error("key is empty: " + k);
        }
        if (m_Items.find(k) != m_Items.end())
        {
            throw std::runtime_error("options key already registered: " + k);
        }
        if (m_LongKeys.find(lk) != m_LongKeys.end())
        {
            throw std::runtime_error("options key already registered: " + lk);
        }

        m_Items[k] = SettingItem(lk, k, desc, std::forward<T>(default_value));
        m_LongKeys.insert(lk);
    }

    template <typename T>
    T get(const std::string &key) const
    {
        auto it = find_option_by_key(key);
        return it->second.value().get<T>();
    }

private:
    std::map<std::string, SettingItem>::const_iterator find_option_by_key(const std::string &k) const throw (std::exception);
    std::string usage(const std::string &program) const;

private:
    std::set<std::string> m_LongKeys;           // long keys
    std::map<std::string, SettingItem> m_Items; // short key and Item
};
