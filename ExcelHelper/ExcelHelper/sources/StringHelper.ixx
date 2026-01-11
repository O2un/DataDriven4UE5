// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
export module IStringHelper;
import <string_view>;
import <ranges>;
import <algorithm>;
import <cctype>;

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