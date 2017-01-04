#pragma once

#include "settings_storage.hpp"

/*
    Programm option class.
*/
class Options
{
public:
    Options() {};
    void parse(int count, const char *const *args);
    void dump() const;

    template <typename T>
    void add(const std::string &lk, const std::string &k, const std::string &desc, T&& default_value)
    {
        if (lk.empty())
        {
            throw std::runtime_error("key is empty: " + k);
        }
        m_Storage.addValueByKey(lk, k, desc, default_value);
    }

    template <typename T>
    const T get(const std::string &key) const
    {
        std::pair<SettingItem &, bool> ret = m_Storage.find_option_by_long_key(key);
        if (!ret.second)
        {
            throw std::runtime_error("no such options: " + key);
        }
        return ret.first.value().get<T>();
    }

private:
    std::string usage(const std::string &program) const;
    int parseFromProgrammOptions(SettingItem &item, int cur_counter, int total_opts, const char *const *args);

private:
    SettingsStorage m_Storage;
};
