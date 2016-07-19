/**
 * Data types and generics used throughout OctoCore.
 *
 * There are six fundamental data types:
 *   bool
 *   int32
 *   int64
 *   double
 *   string
 *   blob
 *
 * There are also five container types:
 *   List       - A list of GenericValues
 *   IntList    - A list of 64-bit integers (or ObjectIds)
 *   StrList    - A list of strings
 *   Map        - A map where the keys are 32-bit integers (FieldId) and the values are generic
 *   StrMap     - A map where the keys are strings and the values are generic
 *
 * Finally, there is a generic container, 'GenericValue', which can contain any of the types
 * described above.
 *
 * Part of OctoCore by Braden MacDonald
 */
#pragma once
#include <cstdint>
#include <string>

#include "messages/CommandData.pb.h"
#include "messages/GenericValue.pb.h"

namespace Octo {

typedef uint32_t FieldId;
typedef int64_t ObjectId;
typedef std::string string;
// Our five container types:
typedef google::protobuf::RepeatedPtrField<GenericValue> List;
typedef google::protobuf::RepeatedField<int64_t> IntList;
typedef google::protobuf::RepeatedPtrField<string> StrList;
typedef google::protobuf::Map<FieldId, GenericValue> Map;
typedef google::protobuf::Map<std::string, GenericValue> StrMap;


/** Wrap: Convert any supported type to a GenericValue
 *  The definition below is the default/fallback that throws a compile error.
 *  Other overloaded definitions are defined by macros below.
 */
template<typename T>
GenericValue wrap(T value) { static_assert(not std::is_same<T, T>(), "Unsupported type passed to wrap()"); };

/** unwrap: Access the typed value inside of a GenericValue. Uses references for complex types. */
template<typename T>
inline T unwrap(GenericValue&);
template<typename T>
inline T unwrap(const GenericValue&);
/** can_unwrap: Determine if the GenericValue contains a value of the given type */
template<typename T>
inline bool can_unwrap(const GenericValue&);
/** unwrap_as: Templated type wrapper that specifies the best type to pass to unwrap<>() if you
 *  want to read a value of type T from a GenericValue.
 *  e.g. unwrap_as<int32_t>::type is int32_t
 *  e.g. unwrap_as<string>::type is const string&
 */
template<typename T>
struct unwrap_as { typedef T type; };
/** mutate_as: Templated type wrapper that specifies the best type to pass to unwrap<>() if you
 *  want to mutate a value of type T.
 *  e.g. unwrap_as<int32_t>::type is int32_t
 *  e.g. unwrap_as<string>::type is string&
 */
template<typename T>
struct mutate_as {
    template <typename U> struct make_reference_mutable { typedef U type; };
    template <typename U> struct make_reference_mutable<const U&> { typedef T& type; };
    typedef typename make_reference_mutable<typename unwrap_as<T>::type>::type type;
};

#define _octo_wrap_basic(Type, accessor_name) \
    inline GenericValue    wrap(Type value) { GenericValue v; v.set_##accessor_name(value); return std::move(v); } \
    template<> inline Type unwrap<Type>(const GenericValue& value) { return value.accessor_name(); } \
    template<> inline bool can_unwrap<Type>(const GenericValue& value) { return value.has_##accessor_name(); }

#define _octo_wrap_container(ContainerType, ContainerWrapperType, accessor_name) \
    inline GenericValue wrap(ContainerType&& value) { \
        ContainerWrapperType wrapper; \
        *wrapper.mutable_entries() = std::move(value); \
        GenericValue v; *v.mutable_##accessor_name() = std::move(wrapper); \
        return std::move(v); \
    } \
    template<> inline const ContainerType& \
    unwrap<const ContainerType&>(const GenericValue& value) { \
        return value.accessor_name().entries(); } \
    template<> inline ContainerType& \
    unwrap<ContainerType&>(GenericValue& value) { \
        return *(value.mutable_##accessor_name()->mutable_entries()); } \
    template<> inline bool \
    can_unwrap<ContainerType>(const GenericValue& value) { \
        return value.has_##accessor_name(); }\
    template<> struct \
    unwrap_as<ContainerType> { typedef const ContainerType& type; };

_octo_wrap_basic(bool, boolean)
_octo_wrap_basic(int32_t, int32)
_octo_wrap_basic(int64_t, int64)
_octo_wrap_basic(double, real)
// Special case for string objects:
/** Wrap a string. Must always contain UTF-8 encoded or 7-bit ASCII text */
inline GenericValue             wrap(const string& value) { GenericValue v; v.set_string(value); return std::move(v); }
inline GenericValue             wrap(const char* value) { GenericValue v; v.set_string(value); return std::move(v); }
template<> inline const string& unwrap<const string&>(const GenericValue& value) { return value.string(); }
template<> inline string&       unwrap<string&>(GenericValue& value) { return *value.mutable_string(); }
template<> inline bool          can_unwrap<string>(const GenericValue& value) { return value.has_string(); }
template<> struct               unwrap_as<string> { typedef const string& type; };
// TODO: blob/bytes
_octo_wrap_container(List, ListValue, list)
_octo_wrap_container(IntList, Int64ListValue, int_list)
_octo_wrap_container(StrList, StringListValue, str_list)
_octo_wrap_container(Map, MapValue, map)
_octo_wrap_container(StrMap, SMapValue, str_map)

} // namespace Octo
