module;
#include "pch.h"
#include <map>

export module ContainerHelper;

export namespace CommonHelper
{
    template <typename T>
    concept IsMapContainer = requires
    {
        typename T::key_type;
        typename T::mapped_type;
        { std::declval<T>().find(std::declval<typename T::key_type>()) } -> std::derived_from<typename T::const_iterator>;
        { std::declval<T>().end() } -> std::derived_from<typename T::const_iterator>;
    };

    template<IsMapContainer M, typename K, typename V>
        requires std::convertible_to<K,typename M::key_type> && std::convertible_to<V,typename M::mapped_type>
    V GetValueOrDefault(const M& map, const K& key, const V& defaultValue) 
    {
        if (auto it = map.find(key); it != map.end()) {
            return it->second;
        }
        return defaultValue;
    }
}