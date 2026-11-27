// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
// 로더(JsonParser)와 데이터 API(DataApi)가 같이 쓰는 셀 값 해석 규칙. JsonParser.cpp에서 옮겨 왔다 (동작 동일).
module;
#include "OpenXLSX.hpp"
#include <array>
#include <charconv>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

export module ICellText;
import IStringHelper;

// 내보내는 inline 함수가 부르므로 내부 연결(익명 namespace)이 아니라 모듈 연결로 둔다
namespace CellTextDetail
{
    inline bool HasNegativeExponent(std::string_view raw)
    {
        const size_t exponent = raw.find_first_of("eE");
        return std::string_view::npos != exponent && exponent + 1 < raw.size() && raw[exponent + 1] == '-';
    }
}

export namespace CellText
{
    // double을 로더가 내보내는 문자열로 바꾼다 (왕복 가능한 최단 고정소수점 표현).
    // 지수 표기(1e-09)는 UE FString::IsNumeric이 숫자로 보지 않아 double/float 변환이 실패하므로 fixed 형식을 쓴다.
    inline std::string DoubleToLoaderText(double number)
    {
        std::array<char, 512> buffer{};   // fixed 형식 최장(DBL_MAX 309자리, 최소 비정규수 소수 약 330자리)보다 크다
        auto [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number, std::chars_format::fixed);
        if (ec == std::errc{})
        {
            return std::string(buffer.data(), end);
        }
        return std::to_string(number);
    }

    // 셀 값을 JSON 문자열 값으로 바꾼다.
    // 실수 셀은 OpenXLSX getString()(std::to_string, 소수 6자리 고정) 대신 DoubleToLoaderText를 쓴다.
    inline std::string CellValueToString(const OpenXLSX::XLCellValueProxy& value)
    {
        if (value.type() == OpenXLSX::XLValueType::Float)
        {
            double number = value.get<double>();
            if (false == std::isfinite(number))
            {
                // 범위를 넘는 값(예: 1.8e308)은 OpenXLSX가 오류 값으로 둔다 -> 기존처럼 getString() 결과("#NUM!")를 쓴다
                return value.getString();
            }
            return DoubleToLoaderText(number);
        }

        // 정수(int64 -> 그대로), 불리언(true/false), 문자열, 오류 값은 기존과 같다
        return value.getString();
    }

    // 범위 검사 대상 컬럼 (2행 타입 기준)
    enum class NumericColumn { None, Int32, Int64, Float, Double };

    inline NumericColumn ToNumericColumn(std::string_view key, std::string_view typeName)
    {
        // _group/_id는 FStaticDataKey(int32)로 들어간다
        if (key == "_group" || key == "_id")
        {
            return NumericColumn::Int32;
        }
        if (CommonHelper::Equals(typeName, "int") || CommonHelper::Equals(typeName, "key"))
        {
            return NumericColumn::Int32;
        }
        if (CommonHelper::Equals(typeName, "long"))
        {
            return NumericColumn::Int64;
        }
        if (CommonHelper::Equals(typeName, "float"))
        {
            return NumericColumn::Float;
        }
        if (CommonHelper::Equals(typeName, "double"))
        {
            return NumericColumn::Double;
        }
        return NumericColumn::None;
    }

    // 숫자 셀의 원문(<v> 텍스트)을 꺼낸다. OpenXLSX 공개 API XLCell::print(셀 XML 출력)를 쓴다.
    // 숫자 셀(t 없음 또는 t="n")이 아니거나 <v>가 없으면 nullopt.
    inline std::optional<std::string> ReadRawNumberText(const OpenXLSX::XLCell& cell)
    {
        std::ostringstream xml;
        cell.print(xml);
        const std::string text = xml.str();

        const size_t tagEnd = text.find('>');
        if (std::string::npos == tagEnd)
        {
            return std::nullopt;
        }

        const std::string_view tag(text.data(), tagEnd);
        if (const size_t typeAttr = tag.find(" t=\""); std::string_view::npos != typeAttr)
        {
            const size_t typeBegin = typeAttr + 4;
            const size_t typeEnd = tag.find('"', typeBegin);
            if (std::string_view::npos == typeEnd || tag.substr(typeBegin, typeEnd - typeBegin) != "n")
            {
                return std::nullopt;
            }
        }

        size_t valueBegin = text.find("<v>", tagEnd);
        if (std::string::npos == valueBegin)
        {
            return std::nullopt;
        }
        valueBegin += 3;
        const size_t valueEnd = text.find("</v>", valueBegin);
        if (std::string::npos == valueEnd)
        {
            return std::nullopt;
        }
        return text.substr(valueBegin, valueEnd - valueBegin);
    }

    // 너무 큰 수 판정. 데이터에 넣으면 안 되면 사유를 돌려준다 (호출부가 0으로 바꾸고 오류 로그).
    // ① 원문과 OpenXLSX가 읽은 값이 다름: '.' 없는 지수 표기(1e+20)를 OpenXLSX가 정수로 분류해 1로 읽는 경우,
    //    int64를 넘는 정수 원문이 잘리는 경우, 원문이 double 범위를 넘는 경우
    // ② 컬럼 타입 범위 초과: int(키 포함) = int32, long = int64, float = float 유한 범위 (double 범위 초과는 ①에 포함)
    inline std::optional<std::string> FindTooLargeNumber(const OpenXLSX::XLCellValueProxy& value, std::string_view raw, NumericColumn column)
    {
        double rawNumber = 0.0;
        const char* rawEnd = raw.data() + raw.size();
        const auto [parsedEnd, ec] = std::from_chars(raw.data(), rawEnd, rawNumber);
        if (ec == std::errc::invalid_argument || parsedEnd != rawEnd)
        {
            return std::nullopt;   // 숫자 원문이 아니면 검사하지 않는다 (기존 동작)
        }
        if (ec == std::errc::result_out_of_range)
        {
            if (CellTextDetail::HasNegativeExponent(raw))
            {
                return std::nullopt;   // 아주 작은 수(언더플로)는 0에 가까운 값 -> 기존 동작 유지
            }
            return std::string("double 범위를 넘습니다");
        }

        const auto valueType = value.type();
        std::optional<double> readNumber;
        if (valueType == OpenXLSX::XLValueType::Integer)
        {
            readNumber = static_cast<double>(value.get<int64_t>());
        }
        else if (valueType == OpenXLSX::XLValueType::Float)
        {
            readNumber = value.get<double>();
        }
        if (false == readNumber.has_value() || *readNumber != rawNumber)
        {
            return std::string("원문(지수 표기 등)을 정확한 값으로 읽을 수 없습니다");
        }

        switch (column)
        {
        case NumericColumn::Int32:
        {
            const double truncated = std::trunc(rawNumber);   // UE는 Atoi64로 소수부를 버린다
            if (truncated < static_cast<double>(INT32_MIN) || static_cast<double>(INT32_MAX) < truncated)
            {
                return std::string("int 범위(-2147483648 ~ 2147483647)를 넘습니다. 더 큰 정수는 long 타입을 쓰세요");
            }
            break;
        }
        case NumericColumn::Int64:
        {
            // 정수 원문은 int64로 직접 파싱한다 (INT64_MAX는 double로 바꾸면 2^63으로 반올림되어 오판한다)
            int64_t rawInteger = 0;
            const auto [intEnd, intEc] = std::from_chars(raw.data(), rawEnd, rawInteger);
            if (intEc == std::errc{} && intEnd == rawEnd)
            {
                break;
            }
            // 소수·지수 원문: int64 경계(-2^63, 2^63)는 double로 정확히 표현된다
            const double truncated = std::trunc(rawNumber);
            if (truncated < -9223372036854775808.0 || 9223372036854775808.0 <= truncated)
            {
                return std::string("long 범위(-9223372036854775808 ~ 9223372036854775807)를 넘습니다");
            }
            break;
        }
        case NumericColumn::Float:
            if (FLT_MAX < std::fabs(rawNumber))
            {
                return std::string("float 범위를 넘습니다");
            }
            break;
        default:
            break;
        }
        return std::nullopt;
    }

    // 로더가 이 셀을 내보낼 문자열 (CellValueToString + 너무 큰 수 -> "0"). reason이 있으면 사유를 채운다.
    inline std::string LoaderCellText(const OpenXLSX::XLCell& cell, NumericColumn column, std::string* reason = nullptr, std::string* raw = nullptr)
    {
        const auto& value = cell.value();
        std::string text = CellValueToString(value);

        const auto valueType = value.type();
        if (valueType == OpenXLSX::XLValueType::Integer || valueType == OpenXLSX::XLValueType::Float || valueType == OpenXLSX::XLValueType::Error)
        {
            if (const auto rawText = ReadRawNumberText(cell))
            {
                if (const auto tooLarge = FindTooLargeNumber(value, *rawText, column))
                {
                    if (reason) *reason = *tooLarge;
                    if (raw) *raw = *rawText;
                    text = "0";
                }
            }
        }
        return text;
    }
}
