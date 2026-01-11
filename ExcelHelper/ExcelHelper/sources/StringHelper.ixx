module;
#include "pch.h"
#include <string_view>
#include <ranges>
#include <algorithm>
#include <cctype>

export module StringHelper;
export namespace CommonHelper
{
    inline bool Equals(std::string_view lhs, std::string_view rhs, bool ignoreCase = true) 
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        if (!ignoreCase)
        {
            return lhs == rhs;
        }
        return std::ranges::equal(lhs, rhs, [](char a, char b) {
            return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
            });
    }
}