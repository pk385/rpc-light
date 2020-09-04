#pragma once

#include <string>
#include <stdexcept>

namespace rpc_light
{
    class ex_method_used : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_method_used(const std::string_view &data = "") : std::runtime_error("Method name unavailable."), m_data(data) {}
    };
    class ex_bad_params : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_bad_params(const std::string_view &data = "") : std::runtime_error("Invalid method parameters."), m_data(data) {}
    };
    class ex_bad_method : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_bad_method(const std::string_view &data = "") : std::runtime_error("Method not found."), m_data(data) {}
    };
    class ex_parse_error : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_parse_error(const std::string_view &data = "") : std::runtime_error("JSON parse error."), m_data(data) {}
    };
    class ex_bad_request : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_bad_request(const std::string_view &data = "") : std::runtime_error("Invalid request."), m_data(data) {}
    };
    class ex_internal_error : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_internal_error(const std::string_view &data = "") : std::runtime_error("Internal error."), m_data(data) {}
    };
    class ex_unknown : public std::runtime_error
    {
        std::string m_data;

    public:
        const std::string data() const { return m_data; }
        ex_unknown(const std::string_view &data = "") : std::runtime_error("Unknown error occured."), m_data(data) {}
    };
} // namespace rpc_light