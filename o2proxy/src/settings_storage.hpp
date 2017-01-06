#pragma once

#include <map>
#include <set>

#include "settings.hpp"

class SettingsStorage
{
public:
    template <typename T>
    void addValueByKey(const std::string &lk, const std::string &k, const std::string &desc, T&& default_value)
    {
        if (m_Items.find(lk) != m_Items.end())
        {
            throw std::runtime_error("options key already registered: " + lk);
        }

        m_Items[lk] = SettingItem(lk, k, desc, std::forward<T>(default_value));
    }

    std::pair<SettingItem&, bool> find_option_by_long_key(const std::string &lk) const noexcept;
    const SettingItem& find_option_by_short_key(const std::string &k) const throw (std::exception);
    
    const std::map<std::string, SettingItem>& items() const { return  m_Items; }

private:
    std::map<std::string, SettingItem> m_Items; // long key and Item
};
