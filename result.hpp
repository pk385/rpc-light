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
        const bool m_has_error;
        const bool m_is_batch;
        const std::string m_string;
        std::variant<response_t, std::vector<response_t>> m_response;

    public:
        result_t(response_t &response, const bool &has_error = false)
            : m_response(response), m_has_error(has_error), m_is_batch(false) {}

        result_t(response_t &response, const std::string_view &str, const bool &has_error = false)
            : m_response(response), m_string(str), m_has_error(has_error), m_is_batch(false) {}

        result_t(std::vector<response_t> &response, const bool &has_error = false)
            : m_response(response), m_has_error(has_error), m_is_batch(true) {}

        result_t(std::vector<response_t> &response, const std::string_view &str, const bool &has_error = false)
            : m_response(response), m_string(str), m_has_error(has_error), m_is_batch(true) {}

        const bool has_error() const
        {
            return m_has_error;
        }

        const bool is_batch() const
        {
            return m_is_batch;
        }

        const std::vector<response_t> get_batch()
        {
            return std::get<std::vector<response_t>>(m_response);
        }

        const response_t get_response() const
        {
            return std::get<response_t>(m_response);
        }

        const auto get_response_str() const
        {
            return m_string;
        }
    };
} // namespace rpc_light