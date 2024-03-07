#pragma once
#include <boost/describe.hpp>

// Various macros for data serialization

#define GLOBED_SERIALIZABLE_ENUM BOOST_DESCRIBE_ENUM
#define GLOBED_SERIALIZABLE_STRUCT(name, ...) BOOST_DESCRIBE_STRUCT(name, (), __VA_ARGS__)
#define GLOBED_SERIALIZABLE_CLASS(name, ...) BOOST_DESCRIBE_CLASS(name, __VA_ARGS__)
#define GLOBED_VERIFY_SERIALIZED_STRUCT(name) static_assert(boost::describe::has_describe_members<name>::value, "struct is not described")
