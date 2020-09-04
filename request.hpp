#pragma once

#include "exceptions.hpp"
#include "value.hpp"
#include "aliases.hpp"

#include <string>

namespace rpc_light
{
    class request_t
    {
         bool m_is_notif;
         value_t m_id;
         params_t m_params;
         std::string m_method;

    public:
        request_t(const std::string_view &method_name, const params_t &params)
            : m_method(method_name), m_params(params), m_id(), m_is_notif(true) {}

        request_t(const std::string_view &method_name, const params_t &params, const value_t &id)
            : m_method(method_name), m_params(params), m_id(id), m_is_notif(false) {}

        const std::string get_method() const
        {
            return m_method;
        }

        const bool is_notification() const
        {
            return m_is_notif;
        }

        const params_t get_params() const
        {
            return m_params;
        }

        const value_t get_id() const
        {
            return m_id;
        }
    };
} // namespace rpc_light