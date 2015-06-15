
//Parts of the code in this file are copied or adopted from utf8-cpp, which had the following copyright notice.

    // Copyright 2006 Nemanja Trifunovic

    /*
    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
    */

#pragma once

#include "dfgDefs.hpp"
#include "utf/utf8_cpp/utf8.h"
#include "utf/utfBom.hpp"
#include "dfgBase.hpp" // For byteorder
#include "io/textEncodingTypes.hpp"
#include <iterator>
#include <limits>
#include <type_traits>
#include "dfgAssert.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(utf) {

    namespace DFG_DETAIL_NS
    {
        template <class Iter_T, bool HasVoidValueType> struct EffectiveIteratorValueTypeHelper
        {
            typedef typename Iter_T::value_type type;
        };

        template <class Iter_T> struct EffectiveIteratorValueTypeHelper<Iter_T, true>
        {
            typedef typename Iter_T::container_type::value_type type;
        };

        // Defines 'type' as
        //	1. std::iterator_traits<Iter_T>::value_type, if Iter_T has non-void value_type
        //	2. typename Iter_T::container_type::value_type, if Iter_T has void value type (e.g. in case of back inserters)
        // For related code, HAS_TYPEDEF in http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence/19815793#19815793
        template <class Iter_T> struct EffectiveIteratorValueType
        {
            static const bool HasVoidValueType = std::is_same<void, typename std::iterator_traits<Iter_T>::value_type>::value;
            typedef typename EffectiveIteratorValueTypeHelper<Iter_T, HasVoidValueType>::type type;
        };

        // Specialization for T*
        template <class T> struct EffectiveIteratorValueType<T*>
        {
            typedef T type;
        };
    } // DFG_DETAIL_NS

    const uint32 INVALID_CODE_POINT = utf8::internal::CODE_POINT_MAX + 1;

    struct UnconvertableCpHandlerParam
    {
        explicit UnconvertableCpHandlerParam(uint32 c) :
            cp(c)
        {}
        uint32 cp;
    };

    inline uint32 defaultUnconvertableCpHandler(UnconvertableCpHandlerParam param)
    {
        DFG_UNUSED(param);
        return '?';
    }

template <typename octet_iterator>
inline typename std::iterator_traits<octet_iterator>::difference_type
sequence_length(octet_iterator lead_it)
{
    uint8_t lead = utf8::internal::mask8(*lead_it);
    if (lead < 0x80)
        return 1;
    else if ((lead >> 5) == 0x6)
        return 2;
    else if ((lead >> 4) == 0xe)
        return 3;
    else if ((lead >> 3) == 0x1e)
        return 4;
    else
        return 0;
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it, const octet_iterator& itEnd)
{
    using utf8::internal::mask8;
    if (it == itEnd)
        return INVALID_CODE_POINT;
    uint32_t cp = mask8(*it);
    typename std::iterator_traits<octet_iterator>::difference_type length = sequence_length(it);
    if (std::distance(it, itEnd) < length)
    {
        it = itEnd;
        return INVALID_CODE_POINT;
    }
    switch (length) {
    case 1:
        break;
    case 2:
        it++;
        cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
        break;
    case 3:
        ++it;
        cp = ((cp << 12) & 0xffff) + ((mask8(*it) << 6) & 0xfff);
        ++it;
        cp += (*it) & 0x3f;
        break;
    case 4:
        ++it;
        cp = ((cp << 18) & 0x1fffff) + ((mask8(*it) << 12) & 0x3ffff);
        ++it;
        cp += (mask8(*it) << 6) & 0xfff;
        ++it;
        cp += (*it) & 0x3f;
        break;
    }
    ++it;
    return cp;
}

// Code point to UTF-8
// TODO: test, especially the byte order handling
template <typename IterUtf_T>
void cpToUtf(const uint32 cp, IterUtf_T result, std::integral_constant<size_t, 8>, ByteOrder boDest = ByteOrderHost)
{
    DFG_UNUSED(boDest);
    auto outputCharSize = sizeof(typename DFG_DETAIL_NS::EffectiveIteratorValueType<IterUtf_T>::type);
    if (outputCharSize == 1)
        utf8::unchecked::append(cp, result);
}

// Code point to UTF-16
// TODO: test, especially the byte order handling
template <typename IterUtf_T>
void cpToUtf(const uint32 cp, IterUtf_T result, std::integral_constant<size_t, 16>, ByteOrder boDest)
{
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable:4244) // Conversion warnings.
    #pragma warning(disable:4127) // Conditional expression is constant
#endif
    const auto outputCharSize = sizeof(typename DFG_DETAIL_NS::EffectiveIteratorValueType<IterUtf_T>::type);
    if (outputCharSize == 2)
    {
        if (cp > 0xffff) //make a surrogate pair
        {
            *result++ = byteSwap(static_cast<uint16_t>((cp >> 10) + utf8::internal::LEAD_OFFSET), ByteOrderHost, boDest);
            *result++ = byteSwap(static_cast<uint16_t>((cp & 0x3ff) + utf8::internal::TRAIL_SURROGATE_MIN), ByteOrderHost, boDest);
        }
        else
            *result++ = byteSwap(static_cast<uint16_t>(cp), ByteOrderHost, boDest);
    }
    else
        DFG_ASSERT_WITH_MSG(false, "Iterator value type should be two bytes wide.");
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
}

// Code point to UTF-32
// TODO: test, especially the byte order handling
template <typename IterUtf_T>
void cpToUtf(const uint32 cp, IterUtf_T result, std::integral_constant<size_t, 32>, ByteOrder boDest)
{
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable:4242) // Conversion warnings.
    #pragma warning(disable:4244) // Conversion warnings.
    #pragma warning(disable:4127) // Conditional expression is constant
#endif
    const auto outputCharSize = sizeof(typename DFG_DETAIL_NS::EffectiveIteratorValueType<IterUtf_T>::type);
    //DFG_STATIC_ASSERT(outputCharSize == 4, "Iterator value type should be four bytes wide.");
    if (outputCharSize == 4)
        *result++ = byteSwap(cp, ByteOrderHost, boDest);
    else
        DFG_ASSERT_WITH_MSG(false, "Iterator value type should be four bytes wide.");
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
}

template <typename IterUtf_T>
void cpToUtf(const uint32 cp, IterUtf_T result, size_t encodedChSize, ByteOrder boDest)
{
    size_t outputTypeSize = sizeof(typename DFG_DETAIL_NS::EffectiveIteratorValueType<IterUtf_T>::type);
    if (outputTypeSize != 1 && outputTypeSize != encodedChSize)
    {
        DFG_ASSERT_WITH_MSG(false, "This function expects either byte output of output whose character size matches with encoding.");
        return;
    }

    if (encodedChSize == 1)
        cpToUtf(cp, result, std::integral_constant<size_t, 8>(), boDest);
    else if (encodedChSize == 2)
    {
        if (outputTypeSize == 2)
            cpToUtf(cp, result, std::integral_constant<size_t, 16>(), boDest);
        else // byte output
        {
            std::vector<uint16> vec; // TODO: replace with static capacity version.
            cpToUtf(cp, std::back_inserter(vec), std::integral_constant<size_t, 16>(), boDest);
            for (size_t i = 0; i < vec.size() * sizeof(uint16); ++i)
            {
                *result++ = *(reinterpret_cast<const char*>(vec.data()) + i);
            }
        }
    }
    else if (encodedChSize == 4)
    {
        if (outputTypeSize == 4)
            cpToUtf(cp, result, std::integral_constant<size_t, 32>(), boDest);
        else // byte output
        {
            uint32 val;
            cpToUtf(cp, &val, std::integral_constant<size_t, 32>(), boDest);
            auto p = reinterpret_cast<const char*>(&val);
            *result++ = *p++;
            *result++ = *p++;
            *result++ = *p++;
            *result++ = *p++;
        }
    }
    else
        DFG_ASSERT(false); // Should not reach here.
}

template <typename IterUtf_T>
void cpToEncoded(const uint32 cp, IterUtf_T result, DFG_MODULE_NS(io)::TextEncoding encoding)
{
    using namespace DFG_MODULE_NS(io);

    //const size_t outputTypeSize = sizeof(typename DFG_DETAIL_NS::EffectiveIteratorValueType<IterUtf_T>::type);

    if (!isUtfEncoding(encoding)) // Currently only utf is implemented.
    {
        DFG_ASSERT(false);
        return;
    }

    const ByteOrder bo = (isBigEndianEncoding(encoding)) ? ByteOrderBigEndian : ByteOrderLittleEndian;
    const auto baseChSize = baseCharacterSize(encoding);

    cpToUtf(cp, result, baseChSize, bo);
}


template <typename u16bit_iterator, typename BswapFunc>
uint32_t readUtf16CharAndAdvance(u16bit_iterator& start, u16bit_iterator end, BswapFunc bswap)
{
    if (start == end)
        return INVALID_CODE_POINT;
    DFG_STATIC_ASSERT(sizeof(*start) == 2, "Expecting iterator to return 16-bit values");
    uint32_t cp = utf8::internal::mask16(bswap(*start++));
    
    // Take care of surrogate pairs first
    if (utf8::internal::is_lead_surrogate(cp))
    {
        if (start == end)
            return INVALID_CODE_POINT;
        uint32_t trail_surrogate = utf8::internal::mask16(bswap(*start++));
        cp = (cp << 10) + trail_surrogate + utf8::internal::SURROGATE_OFFSET;
    }
    return cp;
}

template <typename u32bit_iterator, typename BswapFunc>
uint32_t readUtf32CharAndAdvance(u32bit_iterator& start, u32bit_iterator end, BswapFunc bswap)
{
    DFG_STATIC_ASSERT(sizeof(*start) == 4, "Expecting iterator to return 32-bit values");

    if (start == end)
        return INVALID_CODE_POINT;
    return bswap(*start++);
}

template <typename CharIterator, typename BswapFunc>
uint32_t readUtfCharAndAdvanceImpl(CharIterator& start, const CharIterator end, BswapFunc, std::integral_constant<int, 1>)
{
    return next(start, end);
}

template <typename CharIterator, typename BswapFunc>
uint32_t readUtfCharAndAdvanceImpl(CharIterator& start, const CharIterator end, BswapFunc bswap, std::integral_constant<int,2>)
{
    return readUtf16CharAndAdvance(start, end, bswap);
}

template <typename CharIterator, typename BswapFunc>
uint32_t readUtfCharAndAdvanceImpl(CharIterator& start, const CharIterator end, BswapFunc bswap, std::integral_constant<int, 4>)
{
    return readUtf32CharAndAdvance(start, end, bswap);
}

template <typename CharIterator, typename BswapFunc>
uint32_t readUtfCharAndAdvance(CharIterator& start, const CharIterator end, BswapFunc bswap)
{
    return readUtfCharAndAdvanceImpl(start, end, bswap, std::integral_constant<int, sizeof(*start)>());
}

// Converts given iterable of latin1 values to utf8 representation. When code point is out of range [0, 255],
// UnconvertableHandler will be invoked.
// TODO: test filling
template <class Iterable_T, class OutIter_T, class UnconvertableHandler_T>
void utf8ToLatin1Iter(const Iterable_T& source, OutIter_T dest, UnconvertableHandler_T uch = defaultUnconvertableCpHandler)
{
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    if (iter == iterEnd)
        return;
    for (; iter != iterEnd;)
    {
        uint32 cp = next(iter, iterEnd);
        if (cp < 256)
            *(dest++) = char(cp);
        else
            *(dest++) = static_cast<char>(uch(UnconvertableCpHandlerParam(cp)));
    }
}

// Convert iterable utf8 to latin1 and returns the string.
// TODO: test filling
template <class Iterable_T, class UnconvertableHandler_T>
std::string utf8ToLatin1(const Iterable_T& source, UnconvertableHandler_T uch)
{
    std::string s;
    utf8ToLatin1Iter(source, std::back_inserter(s), uch);
    return s;
}

// Convert iterable utf8 to latin1 and returns the string.
// TODO: test filling
template <class Iterable_T>
std::string utf8ToLatin1(const Iterable_T& source)
{
    return utf8ToLatin1(source, defaultUnconvertableCpHandler);
}

// TODO: test
template <class IterableUtf8_T, class IterOutUtf16_T>
void utf8To16Native(const IterableUtf8_T& source, IterOutUtf16_T dest)
{
    utf8::utf8to16(std::begin(source), std::end(source), dest);
}

// TODO: test
template <class IterableUtf8_T, class IterOutUtf32_T>
void utf8To32Native(const IterableUtf8_T& source, IterOutUtf32_T dest)
{
    utf8::utf8to32(std::begin(source), std::end(source), dest);
}

// Converts utfx-iterable to fixed size string, either Latin1, UCS-2 or UCS-4.
// TODO: test, especially the byte order handling
// TODO: DestType is redundant in many cases; it is there because getting output type from back_inserters is not 
//       the same as getting it from ordinary iterators. TODO: remove it by using EffectiveIteratorValueType.
template <class DestType, class IterableUtf_T, class IterOut_T, class UnconvertableHandler_T>
void utfToFixedChSizeStrIter(const IterableUtf_T& source, IterOut_T dest, ByteOrder boSource, ByteOrder boDest, UnconvertableHandler_T uch)
{
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    if (iter == iterEnd)
        return;
    typedef decltype(*iter) SourceT;
    typedef typename std::make_unsigned<DestType>::type UnsignedDestType;
    const auto sourceBo = [&](SourceT ch){return byteSwap(ch, boSource, ByteOrderHost); };
    for (; iter != iterEnd;)
    {
        uint32 cp = readUtfCharAndAdvanceImpl(iter, iterEnd, sourceBo, std::integral_constant<int, sizeof(*iter)>());
        if (cp <= (std::numeric_limits<UnsignedDestType>::max)())
            *(dest++) = byteSwap(static_cast<UnsignedDestType>(cp), ByteOrderHost, boDest);
        else
            *(dest++) = static_cast<UnsignedDestType>(uch(UnconvertableCpHandlerParam(cp)));
    }
}

// Converts utf8-iterable to fixed size string, either Latin1, UCS-2 or UCS-4.
// TODO: test, especially the byte order handling
template <class DestType, class IterableUtf8_T, class IterOut_T, class UnconvertableHandler_T>
void utf8ToFixedChSizeStrIter(const IterableUtf8_T& source, IterOut_T dest, ByteOrder bo, UnconvertableHandler_T uch)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 1, "Element type of source is expected to have size of single byte");
    utfToFixedChSizeStrIter<DestType>(source, dest, ByteOrderHost, bo, uch);
}

// Overload for convenience.
template <class Char_T, class IterableUtf8_T, class UnconvertableHandler_T>
std::basic_string<Char_T> utf8ToFixedChSizeStr(const IterableUtf8_T& source, ByteOrder bo, UnconvertableHandler_T uch)
{
    std::basic_string<Char_T> str;
    utf8ToFixedChSizeStrIter<Char_T>(source, std::back_inserter(str), bo, uch);
    return str;
}

// Overload for convenience.
template <class Char_T, class IterableUtf8_T>
std::basic_string<Char_T> utf8ToFixedChSizeStr(const IterableUtf8_T& source, ByteOrder bo = ByteOrderHost)
{
    return utf8ToFixedChSizeStr<Char_T>(source, bo, defaultUnconvertableCpHandler);
}

// TODO: test, especially the byte order handling
template <class IterableUtf16_T, class IterOutUtf8_T>
void utf16To8(const IterableUtf16_T& source, IterOutUtf8_T dest, ByteOrder boSource = ByteOrderHost)
{
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    const auto bs = [&](uint16 cp) {return byteSwap(cp, boSource, ByteOrderHost); };
    for (; iter != iterEnd; )
    {
        const auto cp = readUtf16CharAndAdvance(iter, iterEnd, bs);
        ::utf8::unchecked::append(cp, dest);
    }	
}

// TODO: test, especially the byte order handling
template <class IterableUtf16_T, class IterOutUtf32_T>
void utf16To32(const IterableUtf16_T& source, IterOutUtf32_T dest, const ByteOrder boSource = ByteOrderHost, const ByteOrder boDest = ByteOrderHost)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 2, "Element type of source is expected to have size of two bytes");
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    const auto bs = [&](uint16 cp) {return byteSwap(cp, boSource, ByteOrderHost); };
    for (; iter != iterEnd;)
    {
        const auto cp = readUtf16CharAndAdvance(iter, iterEnd, bs);
        *(dest++) = byteSwap(uint32(cp), ByteOrderHost, boDest);
    }
}

// TODO: test, especially the byte order handling
template <class DestType, class IterableUtf16_T, class IterOut_T, class UnconvertableHandler_T>
void utf16ToFixedChSizeStrIter(const IterableUtf16_T& source, IterOut_T dest, UnconvertableHandler_T uch, ByteOrder boSource = ByteOrderHost, ByteOrder boDest = ByteOrderHost)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 2, "Element type of source is expected to have size of two bytes");
    utfToFixedChSizeStrIter<DestType>(source, dest, boSource, boDest, uch);
}

// Overload for convenience.
template <class Char_T, class IterableUtf16_T, class UnconvertableHandler_T>
std::basic_string<Char_T> utf16ToFixedChSizeStr(const IterableUtf16_T& source, ByteOrder boSource, ByteOrder boDest, UnconvertableHandler_T uch)
{
    std::basic_string<Char_T> str;
    utf16ToFixedChSizeStrIter<Char_T>(source, std::back_inserter(str), uch, boSource, boDest);
    return str;
}

// Overload for convenience.
template <class Char_T, class IterableUtf16_T>
std::basic_string<Char_T> utf16ToFixedChSizeStr(const IterableUtf16_T& source, ByteOrder boSource = ByteOrderHost, ByteOrder boDest = ByteOrderHost)
{
    return utf16ToFixedChSizeStr<Char_T>(source, boSource, boDest, defaultUnconvertableCpHandler);
}

// TODO: test, especially the byte order handling
template <class IterableUtf32_T, class IterOutUtf8_T>
void utf32To8(const IterableUtf32_T& source, IterOutUtf8_T dest, const ByteOrder boSource = ByteOrderHost)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 4, "Element type of source is expected to have size of 4 bytes");
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    const auto bs = [&](uint32 cp) {return byteSwap(cp, boSource, ByteOrderHost); };
    for (; iter != iterEnd;)
    {
        const auto cp = readUtf32CharAndAdvance(iter, iterEnd, bs);
        ::utf8::unchecked::append(cp, dest);
    }
}

// TODO: test, especially the byte order handling
template <class IterableUtf32_T, class IterOutUtf16_T>
void utf32To16(const IterableUtf32_T& source, IterOutUtf16_T dest, const ByteOrder boSource = ByteOrderHost, const ByteOrder boDest = ByteOrderHost)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 4, "Element type of source is expected to have size of 4 bytes");
    auto iter = std::begin(source);
    const auto iterEnd = std::end(source);
    const auto bs = [&](uint32 cp) {return byteSwap(cp, boSource, ByteOrderHost); };
    const auto bsDest = [&](uint16 cp) {return byteSwap(cp, ByteOrderHost, boDest); };
    for (; iter != iterEnd;)
    {
        const auto cp = readUtf32CharAndAdvance(iter, iterEnd, bs);
        cpToUtf(cp, dest, std::integral_constant<size_t, 16>(), boDest);
    }
}

// TODO: test, especially the byte order handling
template <class DestType, class IterableUtf32_T, class IterOut_T, class UnconvertableHandler_T>
void utf32ToFixedChSizeStrIter(const IterableUtf32_T& source, IterOut_T dest, ByteOrder boSource, ByteOrder boDest, UnconvertableHandler_T uch)
{
    DFG_STATIC_ASSERT(sizeof(decltype(*std::begin(source))) == 4, "Element type of source is expected to have size of 4 bytes");
    utfToFixedChSizeStrIter<DestType>(source, dest, boSource, boDest, uch);
}

// Overload for convenience.
template <class Char_T, class IterableUtf32_T, class UnconvertableHandler_T>
std::basic_string<Char_T> utf32ToFixedChSizeStr(const IterableUtf32_T& source, ByteOrder boSource, ByteOrder boDest, UnconvertableHandler_T uch)
{
    std::basic_string<Char_T> str;
    utf32ToFixedChSizeStrIter<Char_T>(source, std::back_inserter(str), boSource, boDest, uch);
    return str;
}

// Overload for convenience.
template <class Char_T, class IterableUtf32_T>
std::basic_string<Char_T> utf32ToFixedChSizeStr(const IterableUtf32_T& source, ByteOrder boSource = ByteOrderHost, ByteOrder boDest = ByteOrderHost)
{
    return utf32ToFixedChSizeStr<Char_T>(source, boSource, boDest, defaultUnconvertableCpHandler);
}

// TODO: test, especially the byte order handling
template <class DestChar_T, class Iterable_T, class OutIter_T>
void codePointsToUtf(const Iterable_T& iterableCps, OutIter_T dest, ByteOrder boSource = ByteOrderHost, ByteOrder boDest = ByteOrderHost)
{
    const auto iterEnd = std::end(iterableCps);
    typedef typename std::make_unsigned<typename std::remove_reference<decltype(*iterEnd)>::type>::type UnsignedSourceChar;
    for (auto iter = std::begin(iterableCps); iter != iterEnd; ++iter)
    {
        const auto ch = static_cast<UnsignedSourceChar>(*iter); // This is for negative char values; automatically convert them to uint8.
        const uint32 cp = byteSwap(ch, boSource, ByteOrderHost);
        cpToUtf(cp, dest, std::integral_constant<size_t, sizeof(DestChar_T) * 8>(), boDest);
    }
}

template <class Iterable_T>
std::string codePointsToUtf8(const Iterable_T& iterableCps, ByteOrder boSource = ByteOrderHost)
{
    std::string str;
    codePointsToUtf<char>(iterableCps, std::back_inserter(str), boSource);
    return str;
}

template <class Iterable_T>
std::string codePointsToLatin1(const Iterable_T& iterableCps, ByteOrder boSource = ByteOrderHost)
{
    return utf8ToLatin1(codePointsToUtf8(iterableCps, boSource));
}

// Converts given iterable of latin1 values to utf8 representation and writes them to output iterator dest.
// Negative values are interpreted as uint8-values.
template <class Iterable_T, class OutIter_T>
void latin1ToUtf8(const Iterable_T& iterable, OutIter_T dest)
{
    codePointsToUtf8(iterable, dest);
}

// Overload for convenience returning string with utf8-encoded data.
template <class Iterable_T>
std::string latin1ToUtf8(const Iterable_T& iterable)
{
    return codePointsToUtf8(iterable);
}

}} // module namespace
