// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
export module IStringHelper;
import <string_view>;
import <ranges>;
import <algorithm>;
import <cctype>;
import <string>;
import <Windows.h>;

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

    // 예외 메시지 등 시스템이 만든 narrow 문자열을 UTF-8로 바꾼다.
    // 이미 올바른 UTF-8이면 그대로 두고, 아니면 ACP(한국어 Windows는 CP949)로 보고 변환한다 (std::filesystem_error::what() 등).
    inline std::string ToUtf8FromSystem(std::string_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const int length = static_cast<int>(text.size());
        if (0 < MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), length, nullptr, 0))
        {
            return std::string(text);
        }

        const int wideLength = MultiByteToWideChar(CP_ACP, 0, text.data(), length, nullptr, 0);
        if (wideLength <= 0)
        {
            return std::string(text);
        }
        std::wstring wide(static_cast<size_t>(wideLength), L'\0');
        MultiByteToWideChar(CP_ACP, 0, text.data(), length, wide.data(), wideLength);

        const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, nullptr, 0, nullptr, nullptr);
        if (utf8Length <= 0)
        {
            return std::string(text);
        }
        std::string utf8(static_cast<size_t>(utf8Length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, utf8.data(), utf8Length, nullptr, nullptr);
        return utf8;
    }
}