#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// TODO: file, range, map, unittest

// InstantProxies: 127.0.0.1:1337 * 0,1,2-300,301,302-499; 127.0.0.1:1338 * 501-999,500;
// InstantProxies: 127.0.0.1:1337
// InstantProxies: unix:/var/run/conkee.sock

namespace any
{
    struct address_t
    {
        uint16_t port;
        std::string host;
    };
}


class AnyItem
{
public:
    enum type_t
    {
        UNDEF,
        BOOL,
        INT,
        DOUBLE,
        STRING,
        ADDRESS,
        VECTOR
    };

public:
    AnyItem();
    ~AnyItem();
    AnyItem(const AnyItem &rhs);
    AnyItem& operator=(const AnyItem &rhs);

    type_t type() const { return m_Type; }
    type_t vectorType() const { return m_VectorType; }

    template <typename T> void store(T v)   { store_impl(v, static_cast<T*>(nullptr)); }
    template <typename T> T get() const     { return get_impl(static_cast<T*>(nullptr)); }

    // allow push back only in vectors
    void pushBack(const AnyItem &any);

    friend std::ostream& operator<<(std::ostream& os, const AnyItem& item);

private:
    // this will be specialized in .cpp
    template <typename T> T get_impl(T *) const;

    //this is overload - not specialization
    template <typename T> std::vector<T> get_impl(std::vector<T> *) const
    {
        std::vector<T> ret;
        for (size_t i = 0; i < m_Ptr.v_vector->size(); ++i)
        {
            ret.emplace_back((*m_Ptr.v_vector)[i].get<T>());
        }
        return ret;
    }

    // this will be specialized in .cpp
    template <typename T> void store_impl(T v, T *);

    //this is overload - not specialization
    template <typename T> void store_impl(std::vector<T> v, std::vector<T> *)
    {
        m_Ptr.v_vector = new std::vector<AnyItem>();
        m_Type = VECTOR;

        // it needs to understand what type of elements will stored in vector
        AnyItem any_tmp;
        any_tmp.store<T>(T());
        m_VectorType = any_tmp.type();

        for (size_t i = 0; i < v.size(); ++i)
        {
            AnyItem any;
            any.store<T>(v[i]);
            pushBack(any);
        }
    }

private:
    void clone(const AnyItem &rhs);
    void cloneAsVector(const AnyItem &rhs);

private:
    union
    {
        bool                    *v_bool;
        int                     *v_int;
        double                  *v_double;
        std::string             *v_string;
        any::address_t          *v_address;
        std::vector<AnyItem>    *v_vector;
    }
    m_Ptr;

    type_t m_Type;              // real type of AnyItem
    type_t m_VectorType;        // real type of elements in case AnyItem is vector
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

    std::string key()       const { return _key; }
    std::string lkey()      const { return _long_key; }
    std::string desc()      const { return _description; }
    const AnyItem& value()  const { return _value; }
    AnyItem& value()              { return _value; }

private:
    std::string _long_key;
    std::string _key;
    std::string _description;
    AnyItem _value;
};
