#include "settings.hpp"

void AnyItem::pushBack(const AnyItem &any)
// push in case Vector
{
    if (m_Type != VECTOR)
    {
        throw std::runtime_error("push back in any which is not vector");
    }

    if (m_VectorType != any.type())
    {
        throw std::runtime_error("push back in any with different type of elements");
    }

    if (m_Ptr.v_vector == nullptr)
    {
         throw std::runtime_error("push back into any which is not inicialized");
    }

    m_Ptr.v_vector->push_back(any);
}

void AnyItem::insertPair(const std::pair<AnyItem, AnyItem> &pair)
// push in case Map
{
    if (m_Type != MAP)
    {
        throw std::runtime_error("insert in any which is not map");
    }

    if (m_MapKeyType != pair.first.type())
    {
        throw std::runtime_error("insert in any with different key type");
    }

    if (m_MapValueType != pair.second.type())
    {
        throw std::runtime_error("insert in any with different value type");
    }

    if (m_Ptr.v_map == nullptr)
    {
         throw std::runtime_error("insert into any which is not inicialized");
    }

    (*m_Ptr.v_map)[pair.first] = pair.second;
}


template <>
bool AnyItem::get_impl<bool>(bool *) const
{
    return *(m_Ptr.v_bool);
}
template <>
void AnyItem::store_impl<bool>(bool v, bool *)
{
    if (m_Ptr.v_bool)
    {
        delete m_Ptr.v_bool;
    }

    m_Ptr.v_bool = new bool(v);
    m_Type = BOOL;
}

template <>
int AnyItem::get_impl<int>(int *) const
{
    return *(m_Ptr.v_int);
}
template <>
void AnyItem::store_impl<int>(int v, int *)
{
    if (m_Ptr.v_int)
    {
        delete m_Ptr.v_int;
    }

    m_Ptr.v_int = new int(v);
    m_Type = INT;
}

template <>
double AnyItem::get_impl<double>(double *) const
{
    return *(m_Ptr.v_double);
}
template <>
void AnyItem::store_impl<double>(double v, double *)
{
    if (m_Ptr.v_double)
    {
        delete m_Ptr.v_double;
    }

    m_Ptr.v_double = new double(v);
    m_Type = DOUBLE;
}


template <>
std::string AnyItem::get_impl<std::string>(std::string *) const
{
    return *(m_Ptr.v_string);
}
template <>
void AnyItem::store_impl<std::string>(std::string v, std::string *)
{
    if (m_Ptr.v_string)
    {
        delete m_Ptr.v_string;
    }

    m_Ptr.v_string = new std::string(v);
    m_Type = STRING;
}
template <>
void AnyItem::store_impl<const char*>(const char *v, const char**)
{
    if (m_Ptr.v_string)
    {
        delete m_Ptr.v_string;
    }

    m_Ptr.v_string = new std::string(v);
    m_Type = STRING;
}

template <>
settings::address_t AnyItem::get_impl<settings::address_t>(settings::address_t *) const
{
    return *(m_Ptr.v_address);
}
template <>
void AnyItem::store_impl<settings::address_t>(settings::address_t v, settings::address_t *)
{
    if (m_Ptr.v_address)
    {
        delete m_Ptr.v_address;
    }

    m_Ptr.v_address = new settings::address_t();
    m_Ptr.v_address->port = v.port;
    m_Ptr.v_address->host = v.host;
    m_Type = ADDRESS;
}

template <>
settings::file_t AnyItem::get_impl<settings::file_t>(settings::file_t *) const
{
    return *(m_Ptr.v_file);
}
template <>
void AnyItem::store_impl<settings::file_t>(settings::file_t v, settings::file_t *)
{
    if (m_Ptr.v_file)
    {
        delete m_Ptr.v_file;
    }

    m_Ptr.v_file = new settings::file_t();
    m_Ptr.v_file->name = v.name;
    m_Ptr.v_file->content = v.content;
    m_Type = FILE;
}

template <>
settings::shard_t AnyItem::get_impl<settings::shard_t>(settings::shard_t *) const
{
    return *(m_Ptr.v_shard);
}
template <>
void AnyItem::store_impl<settings::shard_t>(settings::shard_t v, settings::shard_t *)
{
    if (m_Ptr.v_shard)
    {
        delete m_Ptr.v_shard;
    }

    m_Ptr.v_shard = new settings::shard_t();
    m_Ptr.v_shard->shards = v.shards;
    m_Type = SHARD;
}



AnyItem::AnyItem()
{
    m_Type = UNDEF;
    m_Ptr.v_bool = nullptr;
}

void AnyItem::cloneAsVector(const AnyItem &rhs)
{
    if (m_Ptr.v_vector)
    {
        delete m_Ptr.v_vector;
    }

    m_Type = VECTOR;
    m_VectorType = rhs.m_VectorType;

    m_Ptr.v_vector = new std::vector<AnyItem>();

    const std::vector<AnyItem> v = *(rhs.m_Ptr.v_vector);

    for (size_t i = 0; i < v.size(); ++i)
    {
        AnyItem any;
        any.clone(v[i]);
        pushBack(any);
    }
}

void AnyItem::cloneAsMap(const AnyItem &rhs)
{
    if (m_Ptr.v_map)
    {
        delete m_Ptr.v_map;
    }

    m_Type = MAP;
    m_MapKeyType = rhs.m_MapKeyType;
    m_MapValueType = rhs.m_MapValueType;

    m_Ptr.v_map = new std::map<AnyItem, AnyItem>();

    const std::map<AnyItem, AnyItem> v = *(rhs.m_Ptr.v_map);

    for (auto it = v.begin(); it != v.end(); ++it)
    {
        AnyItem any_key;
        any_key.clone(it->first);

        AnyItem any_val;
        any_val.clone(it->second);

        insertPair(std::pair<AnyItem, AnyItem>(any_key, any_val));
    }
}


void AnyItem::clone(const AnyItem &rhs)
{
    m_Type = rhs.m_Type;
    switch (rhs.m_Type)
    {
        case BOOL:      { store(rhs.get<bool>()); break; }
        case INT:       { store(rhs.get<int>()); break; }
        case DOUBLE:    { store(rhs.get<double>()); break; }
        case STRING:    { store(rhs.get<std::string>()); break; }
        case ADDRESS:   { store(rhs.get<settings::address_t>()); break; }
        case FILE:      { store(rhs.get<settings::file_t>()); break; }
        case SHARD:     { store(rhs.get<settings::shard_t>()); break; }
        case VECTOR:    { cloneAsVector(rhs); break; }
        case MAP:       { cloneAsMap(rhs); break; }
        default:        { throw std::runtime_error("not implemented type"); }
    }
}

AnyItem::AnyItem(const AnyItem &rhs)
{
    m_Ptr.v_bool = nullptr;
    clone(rhs);
}

AnyItem& AnyItem::operator=(const AnyItem &rhs)
{
    m_Ptr.v_bool = nullptr;

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
        case DOUBLE:    { delete m_Ptr.v_double; break; }
        case STRING:    { delete m_Ptr.v_string; break; }
        case ADDRESS:   { delete m_Ptr.v_address; break; }
        case FILE:      { delete m_Ptr.v_file; break; }
        case SHARD:     { delete m_Ptr.v_shard; break; }
        case VECTOR:    { delete m_Ptr.v_vector; break; }
        case MAP:       { delete m_Ptr.v_map; break; }
        default: {}
    }
}

std::ostream& operator<<(std::ostream& os, const AnyItem& item)
{
    switch (item.m_Type)
    {
        case AnyItem::BOOL:      { os << std::boolalpha << item.get<bool>(); break; }
        case AnyItem::INT:       { os << item.get<int>(); break; }
        case AnyItem::DOUBLE:    { os << item.get<double>(); break; }
        case AnyItem::STRING:    { os << item.get<std::string>(); break; }
        case AnyItem::ADDRESS:   { settings::address_t a = item.get<settings::address_t>(); os << a.host << ":" << a.port; break; }
        case AnyItem::FILE:      { settings::file_t f = item.get<settings::file_t>(); os << f.name << ", " << f.content.size() << " bytes"; break; }
        case AnyItem::SHARD:     {
                                    settings::shard_t s = item.get<settings::shard_t>();
                                    os << "total: " << s.shards.size() << "; shards list: ";
                                    for (size_t i = 0; i < s.shards.size(); ++i)
                                    {
                                        if (i != 0) os << ", ";
                                        os << s.shards[i];
                                    }
                                    break;
                                 }
        case AnyItem::VECTOR:    {
                                    os << "[";
                                    for (size_t i = 0; i < item.m_Ptr.v_vector->size(); ++i)
                                    {
                                        if (i != 0) os << ", ";
                                        os << item.m_Ptr.v_vector->at(i);
                                    }
                                    os << "]";
                                    break;
                                 }
        case AnyItem::MAP:    {
                                    int counter = 0;
                                    os << "{";
                                    for (auto it = item.m_Ptr.v_map->begin(); it != item.m_Ptr.v_map->end(); ++it)
                                    {
                                        if (counter++ != 0) os << ", ";
                                        os << it->first << " => " << it->second;
                                    }
                                    os << "}";
                                    break;
                                 }

        default:                 { os << "nil"; break; }
    }
    return os;
}

