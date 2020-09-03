#pragma once

#include "exceptions.hpp"

#include <variant>
#include <string>
#include <vector>
#include <map>

namespace rpc_light
{
    class value_t
    {
        template <typename type, typename variant_type>
        struct can_hold_alt;

        template <typename type, typename... variant_type>
        struct can_hold_alt<type, std::variant<variant_type...>>
            : public std::disjunction<std::is_same<type, variant_type>...>
        {
        };

        template <typename type, typename variant_type>
        struct can_convert_alt;

        template <typename type, typename... variant_type>
        struct can_convert_alt<type, std::variant<variant_type...>>
            : public std::disjunction<std::is_convertible<variant_type, type>...>
        {
        };

        using variant_t = std::variant<std::monostate, std::vector<value_t>,
                                       bool, double, int32_t, int64_t, std::string,
                                       std::map<std::string, value_t>>;

        variant_t m_value;

    public:
        value_t() {}

        template <typename value_type>
        value_t(value_type value) : m_value(value) {}

        const bool has_value() const
        {
            return !std::holds_alternative<std::monostate>(m_value);
        }

        template <typename value_type>
        const value_type get_alt(const bool no_convert = false) const
        {

            if constexpr (can_hold_alt<value_type, variant_t>::value)
                if (std::holds_alternative<value_type>(m_value))
                    return std::get<value_type>(m_value);

            if constexpr (can_convert_alt<value_type, variant_t>::value)
            {
                value_type value;
                bool found = false;
                std::visit([&](auto &&arg) {
                    using type = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_convertible_v<type, value_type>)
                    {
                        value = std::get<type>(m_value);
                        found = true;
                    }
                },
                           m_value);
                if (found)
                    return value;
            }
            throw ex_internal_error("Bad alternative.");
        }

        const auto &get_variant() const
        {
            return m_value;
        }
    }; // namespace rpc_light
} // namespace rpc_light