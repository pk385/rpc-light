#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"

#include <string>

namespace rpc_light
{
    class request_t
    {
        const value_t m_id;
        const std::string m_method;
        const bool m_is_notif, m_named_params, m_has_params;
        const std::variant<null_t, array_t, struct_t> m_params;

    public:
        request_t(const std::string_view &method_name)
            : m_method(method_name), m_is_notif(true),
              m_named_params(false), m_has_params(false) {}

        request_t(const std::string_view &method_name, const value_t &id)
            : m_method(method_name), m_id(id), m_is_notif(false),
              m_named_params(false), m_has_params(false) {}

        template <typename params_type>
        request_t(const std::string_view &method_name, const params_type &params)
            : m_method(method_name), m_params(params), m_is_notif(true), m_has_params(true),
              m_named_params(std::is_same_v<params_type, struct_t>) {}

        template <typename params_type>
        request_t(const std::string_view &method_name, const params_type &params, const value_t &id)
            : m_method(method_name), m_params(params), m_id(id), m_is_notif(false), m_has_params(true),
              m_named_params(std::is_same_v<params_type, struct_t>) {}

        const inline std::string get_method() const
        {
            return m_method;
        }

        const inline bool is_notification() const
        {
            return m_is_notif;
        }

        const inline bool has_params() const
        {
            return m_has_params;
        }

        const inline bool has_named_params() const
        {
            return m_named_params;
        }

        const inline array_t get_params_arr() const
        {
            return std::get<array_t>(m_params);
        }

        const inline struct_t get_params_str() const
        {
            return std::get<struct_t>(m_params);
        }

        const inline value_t get_id() const
        {
            return m_id;
        }
    };
} // namespace rpc_light