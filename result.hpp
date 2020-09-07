#pragma once

#include "exceptions.hpp"
#include "value.hpp"
#include "aliases.hpp"
#include "response.hpp"

#include <string>
#include <variant>
#include <vector>

namespace rpc_light
{
    class result_t
    {
        using batch_t = std::vector<response_t>;
        const std::string m_string;
        const bool m_has_error, m_is_batch, m_has_response;
        const std::variant<null_t, response_t, batch_t> m_response;

    public:
        result_t(const bool &has_error = false)
            : m_has_error(has_error),
              m_is_batch(false), m_has_response(false) {}

        result_t(const std::string_view &str, const bool &has_error = false)
            : m_string(str), m_has_error(has_error),
              m_is_batch(false), m_has_response(false) {}

        template <typename response_type>
        result_t(const response_type &response, const bool &has_error = false)
            : m_response(response), m_has_error(has_error), m_has_response(true),
              m_is_batch(std::is_same_v<response_type, batch_t>) {}

        template <typename response_type>
        result_t(const response_type &response, const std::string_view &str, const bool &has_error = false)
            : m_response(response), m_string(str), m_has_error(has_error), m_has_response(true),
              m_is_batch(std::is_same_v<response_type, batch_t>) {}

        const inline bool has_error() const
        {
            return m_has_error;
        }

        const inline bool has_response() const
        {
            return m_has_response;
        }

        const inline bool is_batch() const
        {
            return m_is_batch;
        }

        const inline batch_t get_batch() const
        {
            return std::get<batch_t>(m_response);
        }

        const inline response_t get_response() const
        {
            return std::get<response_t>(m_response);
        }

        const inline std::string get_response_str() const
        {
            return m_string;
        }
    };
} // namespace rpc_light