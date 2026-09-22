// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INT128_IOSTREAM_HPP
#define BOOST_INT128_IOSTREAM_HPP

#include <boost/int128/int128.hpp>
#include <boost/int128/detail/mini_from_chars.hpp>
#include <boost/int128/detail/mini_to_chars.hpp>
#include <boost/int128/detail/utilities.hpp>
#include <boost/int128/detail/config.hpp>

#ifndef BOOST_INT128_BUILD_MODULE

#include <type_traits>
#include <iostream>
#include <iomanip>
#include <cstring>

#endif

namespace boost {
namespace int128 {

namespace detail {

template <typename T>
struct streamable_overload
{
    static constexpr bool value = std::is_same<T, uint128>::value || std::is_same<T, int128>::value;
};

template <typename T>
BOOST_INT128_INLINE_CONSTEXPR bool is_streamable_overload_v = streamable_overload<T>::value;

} // namespace detail

#if defined(__GNUC__) && __GNUC__ >= 5 && __GNUC__ < 11
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

BOOST_INT128_EXPORT template <typename charT, typename traits, typename LibIntegerType>
auto operator>>(std::basic_istream<charT, traits>& is, LibIntegerType& v)
    -> std::enable_if_t<detail::is_streamable_overload_v<LibIntegerType>, std::basic_istream<charT, traits>&>
{
    charT t_buffer[64] {};
    is >> std::ws >> std::setw(63) >> t_buffer;

    const auto t_buffer_len {std::char_traits<charT>::length(t_buffer)};

    char buffer[64] {};
    auto buffer_start {buffer};

    BOOST_INT128_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        auto first {buffer};
        auto t_first {t_buffer};
        const auto t_buffer_end {t_buffer + detail::strlen(t_buffer)};

        while (t_first != t_buffer_end)
        {
            *first++ = static_cast<char>(*t_first++);
        }
    }
    else
    {
        std::memcpy(buffer, t_buffer, sizeof(t_buffer));
    }

    const auto flags {is.flags()};
    int base {10};
    if (flags & std::ios_base::oct)
    {
        // No prefix is stripped: in base 8 a leading zero is already an ordinary digit,
        // so "017" reads as 15 and "08" reads as 0 leaving the '8' in the stream, which
        // is what num_get does for the builtin types.
        base = 8;
    }
    else if (flags & std::ios_base::hex)
    {
        base = 16;

        // Skip an explicit 0x or 0X prefix, and never a bare leading zero, which
        // would swallow the first digit of a value such as 0f
        if (buffer_start[0] == '0' && (buffer_start[1] == 'x' || buffer_start[1] == 'X'))
        {
            buffer_start += 2;
        }
    }

    const auto prefix_length {static_cast<std::size_t>(buffer_start - buffer)};

    const auto r {detail::from_chars(buffer_start, buffer + detail::strlen(buffer), v, base)};

    // Put back unconsumed characters. Only a strictly negative r means digits were
    // extracted, and then -r digits were consumed on top of any base prefix. Anything
    // else consumed nothing at all, so even the prefix goes back.
    std::size_t consumed {};
    if (r < 0)
    {
        consumed = prefix_length + static_cast<std::size_t>(-r);
    }

    BOOST_INT128_ASSERT(t_buffer_len >= consumed);
    const auto return_chars {static_cast<std::size_t>(t_buffer_len - consumed)};

    for (std::size_t i {}; i < return_chars; ++i)
    {
        is.putback(t_buffer[t_buffer_len - i - 1]);
    }

    // from_chars returns the negated number of characters consumed on success, so
    // anything not negative means no digits were extracted: r == 0 is a first
    // character that is not a digit in the base, and r > 0 is an errno value
    // (EINVAL for an empty input or a sign, EDOM for a value that does not fit).
    // The stream has to report all of those as a failure. This must come after the
    // putback loop: putback fails its own sentry once failbit is set.
    if (r >= 0)
    {
        v = LibIntegerType{};
        is.setstate(std::ios_base::failbit);
    }

    return is;
}

#if defined(__GNUC__) && __GNUC__ >= 5 && __GNUC__ < 11
#  pragma GCC diagnostic pop
#endif

BOOST_INT128_EXPORT template <typename charT, typename traits, typename LibIntegerType>
auto operator<<(std::basic_ostream<charT, traits>& os, const LibIntegerType& v)
    -> std::enable_if_t<detail::is_streamable_overload_v<LibIntegerType>, std::basic_ostream<charT, traits>&>
{
    char buffer[detail::mini_to_chars_buffer_size] {};

    const auto flags {os.flags()};
    int base {10};
    bool uppercase {false};
    if (flags & std::ios_base::oct)
    {
        base = 8;
    }
    else if (flags & std::ios_base::hex)
    {
        base = 16;
    }

    if (flags & std::ios_base::uppercase)
    {
        uppercase = true;
    }

    auto first {detail::mini_to_chars(buffer, v, base, uppercase)};

    // A zero prints as a bare "0" with showbase, the same as the builtin types
    if ((flags & std::ios_base::showbase) && v != 0U)
    {
        if (base == 8)
        {
            *--first = '0';
        }
        else if (base == 16)
        {
            *--first = uppercase ? 'X' : 'x';
            *--first = '0';
        }
    }

    BOOST_INT128_IF_CONSTEXPR (!std::is_same<charT, char>::value)
    {
        charT t_buffer[64U] {};

        auto t_first {t_buffer};
        while (*first != '\0')
        {
            *t_first++ = static_cast<charT>(*first++);
        }

        os << t_buffer;
    }
    else
    {
        os << first;
    }

    return os;
}

} // namespace int128
} // namespace boost

#endif // BOOST_INT128_IOSTREAM_HPP
