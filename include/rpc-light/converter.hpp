#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"

#include <any>
#include <unordered_map>

namespace rpc_light
{
    class converter_t
    {
        using convert_t = std::function<std::any(std::any)>;
        std::unordered_map<std::size_t, convert_t> m_converts;

        template <typename return_type, typename value_type>
        void add_convert_internal(const std::function<return_type(value_type)> &method)
        {
            convert_t expr = [method](const std::any &arg) {
                return method(std::any_cast<std::decay_t<value_type>>(arg));
            };
            m_converts.emplace(typeid(std::decay_t<return_type>(std::decay_t<value_type>)).hash_code(), expr);
        }

    public:
        template <typename method_type>
        void add_convert(const method_type &convert)
        {
            add_convert_internal(std::function(convert));
        }
        template <typename return_type, typename value_type>
        const return_type convert(const value_type &value) const
        {
            if (auto iter = m_converts.find(typeid(return_type(value_type)).hash_code()); iter != m_converts.end())
                return std::any_cast<return_type>(iter->second(value));
            throw ex_internal_error("Bad convert.");
        }
    };
} // namespace rpc_light