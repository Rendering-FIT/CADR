/**
 * @file unicode.h
 * @author Christian Saloň
 */

#pragma once

#include <stdexcept>
#include <cstdint>

namespace vft {

constexpr uint32_t U_BACKSPACE = 0x00000008;
constexpr uint32_t U_CR = 0x0000000d;
constexpr uint32_t U_LF = 0x0000000a;
constexpr uint32_t U_SPACE = 0x00000020;
constexpr uint32_t U_TAB = 0x00000009;

class Unicode {
public:
    static std::u16string utf8ToUtf16(std::u8string input);
    static std::u32string utf8ToUtf32(std::u8string input);

    static std::u8string utf16ToUtf8(std::u16string input);
    static std::u32string utf16ToUtf32(std::u16string input);

    static std::u8string utf32ToUtf8(std::u32string input);
    static std::u16string utf32ToUtf16(std::u32string input);

    static unsigned int getSizeOfUtf8Character(char8_t firstByte);
    static unsigned int getSizeOfUtf16Character(char16_t firstByte);
};

}  // namespace vft
