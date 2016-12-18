#pragma once

#include <string>
#include <memory>
#include <iostream>

class AnyItem
{
public:
    enum type_t
    {
        UNDEF,
        BOOL,
        INT,
        STRING
    };

public:
    AnyItem();
    ~AnyItem();
    AnyItem(const AnyItem &rhs);
    AnyItem& operator=(const AnyItem &rhs);

    type_t type() const { return m_Type; }

    template <typename T> void store(T v);
    template <typename T> T get() const;

    friend std::ostream& operator<<(std::ostream& os, const AnyItem& item);

private:
    void clone(const AnyItem &rhs);

private:
    union
    {
        bool        *v_bool;
        int         *v_int;
        std::string *v_string;
    } m_Ptr;

    type_t m_Type;
};



class SettingItem
{
public:
    SettingItem() {}

    template<typename T>
    SettingItem(const std::string &lk, const std::string &k, const std::string &desc, T &&v) :
        _long_key(lk), _key(k), _description(desc)
    {
        _value.store(std::forward<T>(v));
    }

    std::string key() const { return _key; }
    std::string lkey() const { return _long_key; }
    std::string desc() const { return _description; }
    const AnyItem& value() const { return _value; }

    int parseFromProgrammOptions(int cur_counter, int total_opts, const char *const *args);

private:
    std::string _long_key;
    std::string _key;
    std::string _description;
    AnyItem _value;
};
