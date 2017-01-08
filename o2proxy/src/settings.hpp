#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>


namespace settings
{
    struct address_t
    {
        uint16_t port;
        std::string host;
    };

    struct file_t
    {
        std::string name;
        std::string content;
    };

    struct shard_t
    {
        std::vector<int> shards;
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
        FILE,
        SHARD,
        VECTOR,
        MAP,
    };

public:
    AnyItem();
    ~AnyItem();
    AnyItem(const AnyItem &rhs);
    AnyItem& operator=(const AnyItem &rhs);

    // the order does not make sense in Any
    bool operator<(const AnyItem &rhs) const { return m_Ptr.v_int < rhs.m_Ptr.v_int; }

    type_t type() const { return m_Type; }
    type_t vectorType() const { return m_VectorType; }
    type_t mapKeyType() const { return m_MapKeyType; }
    type_t mapValueType() const { return m_MapValueType; }

    template <typename T> void store(T v)   { store_impl(v, static_cast<T*>(nullptr)); }
    template <typename T> T get() const     { return get_impl(static_cast<T*>(nullptr)); }

    // allow push back only in vectors
    void pushBack(const AnyItem &any);

    // allow insert only in maps
    void insertPair(const std::pair<AnyItem, AnyItem> &pair);

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

    template <typename K, typename T> std::map<K, T> get_impl(std::map<K, T> *) const
    {
        std::map<K, T> ret;
        for (auto it = m_Ptr.v_map->begin(); it != m_Ptr.v_map->end(); ++it)
        {
            std::pair<K, T> p = std::make_pair(it->first.get<K>(), it->second.get<T>());
            ret.insert(p);
            //ret[it->first] = it->second;
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

    template <typename K, typename T> void store_impl(std::map<K, T> v, std::map<K, T> *)
    {
        m_Ptr.v_map = new std::map<AnyItem, AnyItem>();
        m_Type = MAP;

        // it needs to understand what type of elements will stored in vector
        AnyItem any_key;
        any_key.store<K>(K());
        m_MapKeyType = any_key.type();

        AnyItem any_val;
        any_val.store<T>(T());
        m_MapValueType = any_val.type();

        for (auto it = v.begin(); it != v.end(); ++it)
        {
            AnyItem any_key;
            any_key.store<K>(it->first);

            AnyItem any_val;
            any_val.store<T>(it->second);

            insertPair(std::pair<AnyItem, AnyItem>(any_key, any_val));
        }
    }

private:
    void clone(const AnyItem &rhs);
    void cloneAsVector(const AnyItem &rhs);
    void cloneAsMap(const AnyItem &rhs);

private:
    union
    {
        bool                            *v_bool;
        int                             *v_int;
        double                          *v_double;
        std::string                     *v_string;
        settings::address_t             *v_address;
        settings::file_t                *v_file;
        settings::shard_t               *v_shard;
        std::vector<AnyItem>            *v_vector;
        std::map<AnyItem, AnyItem>      *v_map;
    }
    m_Ptr;

    type_t m_Type;              // real type of AnyItem
    type_t m_VectorType;        // real type of elements in case AnyItem is vector
    type_t m_MapKeyType;        // real type of key-elements in map
    type_t m_MapValueType;      // real type of value-elements in map
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
