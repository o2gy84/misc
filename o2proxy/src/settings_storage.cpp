#include "settings_storage.hpp"

const SettingItem& SettingsStorage::find_option_by_long_key(const std::string &lk) const throw (std::exception)
{
    if (lk.empty())
    {
        throw std::runtime_error("request for empty option");
    }

    auto it = m_Items.find(lk);
    if (it == m_Items.end())
    {
        throw std::runtime_error("no such option: " + lk);
    }

    return it->second;
}

const SettingItem& SettingsStorage::find_option_by_short_key(const std::string &k) const throw (std::exception)
{
    if (k.empty())
    {
        throw std::runtime_error("request for empty option");
    }

    for (auto it = m_Items.begin(); it != m_Items.end(); ++it)
    {
        const SettingItem &item = it->second;
        if (item.key() == k)
        {
            return item;
        }
    }

    throw std::runtime_error("no such option: " + k);
}

