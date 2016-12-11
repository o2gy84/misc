#include "settings.hpp"

#include <iostream>

AnyItem::AnyItem()
{
    m_Type = UNDEF;
    m_Ptr.v_bool = nullptr;
}

void AnyItem::clone(const AnyItem &rhs)
{
    m_Type = rhs.m_Type;
    switch (rhs.m_Type)
    {
        case BOOL: { storeBool(rhs.getBool()); break; }
        case INT: { storeInt(rhs.getInt()); break; }
        case STRING: { storeString(rhs.getString()); break; }
        default: { throw std::runtime_error("not implemented"); }
    }
}

AnyItem::AnyItem(const AnyItem &rhs)
{
    clone(rhs);
}

AnyItem& AnyItem::operator=(const AnyItem &rhs)
{
    if (this == &rhs)
    {
        return *this;
    }
    clone(rhs);
    return *this;
}

AnyItem::~AnyItem()
{
    switch (m_Type)
    {
        case BOOL:      { delete m_Ptr.v_bool; break; }
        case INT:       { delete m_Ptr.v_int; break; }
        case STRING:    { delete m_Ptr.v_string; break; }
        default: {}
    }
}

void AnyItem::storeBool(bool v)
{
    m_Ptr.v_bool = new bool(v);
    m_Type = BOOL;
}

void AnyItem::storeInt(int v)
{
    m_Ptr.v_int = new int(v);
    m_Type = INT;
}

void AnyItem::storeString(const std::string &v)
{
    m_Ptr.v_string = new std::string(v);
    m_Type = STRING;
}


std::ostream& operator<<(std::ostream& os, const AnyItem& item)
{
    switch (item.m_Type)
    {
        case AnyItem::BOOL:      { os << std::boolalpha << item.getBool(); break; }
        case AnyItem::INT:       { os << item.getInt(); break; }
        case AnyItem::STRING:    { os << item.getString(); break; }
        default:        { os << "nil"; break; }
    }
    return os;
}




/****************************************************/

int SettingItem::parseFromProgrammOptions(int cur_counter, int total_opts, const char *const *args)
{
    if (_value.type() == AnyItem::BOOL)
    {
        // positional option - just yes or no
        _value.storeBool(true);
        return cur_counter + 1;
    }
    else if (_value.type() == AnyItem::INT)
    {
        // TODO: parse more complex
        // examples: "-l 5", "-l=5", "-l5"
        if (cur_counter + 1 >= total_opts)
        {
            throw std::runtime_error("bad options");
        }

        int v = std::stoi(args[cur_counter + 1]);
        _value.storeInt(v);
        return cur_counter + 2;
    }
    else if (_value.type() == AnyItem::STRING)
    {
        // TODO: parse more complex
        // examples: "-c /etc/config.conf", "-c = /etc/config.conf"
        if (cur_counter + 1 >= total_opts)
        {
            throw std::runtime_error("bad options");
        }

        _value.storeString(args[cur_counter + 1]);
        return cur_counter + 2;
    }

    throw std::runtime_error("bad options");
    return -1;
}
