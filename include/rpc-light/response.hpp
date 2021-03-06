#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"

#include <string>

namespace rpc_light
{
    class response_t
    {
        const int m_code;
        const bool m_is_notif;
        const std::string m_message;
        const value_t m_value, m_id, m_data;

    public:
        response_t(const value_t &value)
            : m_value(value), m_id(), m_code(0), m_is_notif(true) {}

        response_t(const value_t &value, const value_t &id)
            : m_value(value), m_id(id), m_code(0), m_is_notif(false) {}

        response_t(const int &code, const std::string_view &message)
            : m_code(code), m_message(message), m_is_notif(true) {}

        response_t(const int &code, const std::string_view &message, const value_t &id)
            : m_code(code), m_message(message), m_id(id), m_is_notif(false) {}

        response_t(const int &code, const std::string_view &message, const value_t &id, const value_t &data)
            : m_code(code), m_message(message), m_id(id), m_data(data), m_is_notif(false) {}

        const inline value_t get_id() const
        {
            return m_id;
        }

        const inline bool is_notification() const
        {
            return m_is_notif;
        }

        const inline value_t get_value() const
        {
            return m_value;
        }

        const inline int get_code() const
        {
            return m_code;
        }

        const inline value_t get_data() const
        {
            return m_data;
        }

        const inline bool has_error() const
        {
            return m_code != 0;
        }

        const inline std::string get_message() const
        {
            return m_message;
        }
    };
} // namespace rpc_light