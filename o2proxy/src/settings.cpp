#include "settings.hpp"

void AnyItem::pushBack(const AnyItem &any)
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

void AnyItem::clone(const AnyItem &rhs)
{
    m_Type = rhs.m_Type;
    switch (rhs.m_Type)
    {
        case BOOL:      { store(rhs.get<bool>()); break; }
        case INT:       { store(rhs.get<int>()); break; }
        case DOUBLE:    { store(rhs.get<double>()); break; }
        case STRING:    { store(rhs.get<std::string>()); break; }
        case VECTOR:    { cloneAsVector(rhs); break; }
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
        case VECTOR:    { delete m_Ptr.v_vector; break; }
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

        default:                 { os << "nil"; break; }
    }
    return os;
}

