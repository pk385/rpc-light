#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "converter.hpp"

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

        using variant_t = std::variant<null_t, array_t,
                                       bool, double, int32_t, int64_t, std::string,
                                       struct_t>;

        variant_t m_value;
        static inline converter_t m_converter;

    public:
        value_t() {}

        template <typename value_type>
        value_t(const value_type &value) : m_value(value) {}

        template <typename value_type>
        value_t(const std::vector<value_type> &value)
            : m_value(array_t(value.begin(), value.end())) {}

        template <typename value_type>
        value_t(const std::map<std::string, value_type> &value)
            : m_value(struct_t(value.begin(), value.end())) {}

        template <typename value_type>
        const inline bool is_type() const
        {
            return std::holds_alternative<value_type>(m_value);
        }

        const inline bool has_value() const
        {
            return !std::holds_alternative<null_t>(m_value);
        }

        template <typename value_type>
        const value_type get_value(const bool &allow_convert = true) const
        {
            if constexpr (can_hold_alt<value_type, variant_t>::value)
                if (std::holds_alternative<value_type>(m_value))
                    return std::get<value_type>(m_value);

            if (allow_convert)
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
                    else
                    {
                        value = m_converter.convert<value_type>(arg);
                        found = true;
                    }
                },
                           m_value);
                if (found)
                    return value;
            }
            throw ex_internal_error("Bad alternative.");
        }

        const inline auto &get_variant() const
        {
            return m_value;
        }

        static inline auto &get_converter()
        {
            return m_converter;
        }
    };
    static inline auto &global_converter = value_t::get_converter();

} // namespace rpc_light