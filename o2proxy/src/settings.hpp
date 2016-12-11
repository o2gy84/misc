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

    void storeBool(bool v);
    void storeInt(int v);
    void storeString(const std::string &v);

    // TODO: checks for type!
    bool getBool() const { return *(m_Ptr.v_bool); }
    int getInt() const { return *(m_Ptr.v_int); }
    std::string getString() const { return *(m_Ptr.v_string); }

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

    SettingItem(const std::string &lk, const std::string &k, const std::string &desc, bool v) :
        _long_key(lk), _key(k), _description(desc)
    {
        _value.storeBool(v);
    }

    SettingItem(const std::string &lk, const std::string &k, const std::string &desc, int v) :
        _long_key(lk), _key(k), _description(desc)
    {
        _value.storeInt(v);
    }

    SettingItem(const std::string &lk, const std::string &k, const std::string &desc, const std::string &v) :
        _long_key(lk), _key(k), _description(desc)
    {
        _value.storeString(v);
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
