#pragma once

#include "exceptions.hpp"
#include "value.hpp"
#include "aliases.hpp"
#include "response.hpp"
#include "request.hpp"

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <shared_mutex>

namespace rpc_light
{

    class dispatcher_t
    {
        std::map<std::string, method_t> m_methods;
        std::shared_mutex m_mutex;

        template <typename return_type, typename... params_type>
        void add_method_internal(const std::string_view &name, const std::function<return_type(params_type...)> &method)
        {
            add_method_internal(name, method, std::index_sequence_for<params_type...>());
        }

        template <typename return_type, typename... params_type, std::size_t... index>
        void add_method_internal(const std::string_view &name, const std::function<return_type(params_type...)> &method, const std::index_sequence<index...>)
        {
            method_t lambda = [method](const params_t &params) -> value_t {
                constexpr auto params_size = sizeof...(params_type);
                if (params_size != params.size())
                    throw ex_bad_params("Params length mismatch.");
                try
                {
                    if constexpr (!std::is_void_v<return_type>)
                    {
                        if constexpr (params_size > 0)
                            return value_t(method(params[index].get_alt<std::decay_t<params_type>>()...));
                        else
                            return value_t(method());
                    }
                    else
                    {
                        if constexpr (params_size > 0)
                            method(params[index].get_alt<std::decay_t<params_type>>()...);
                        else
                            method();
                        return null_t();
                    }
                }
                catch (std::bad_variant_access e)
                {
                    throw ex_bad_params("Invalid param types.");
                }
            };
            add_method(name, lambda);
        }

    public:
        dispatcher_t() {}

        void add_method(const std::string_view &name, const method_t &method)
        {
            {
                std::shared_lock lock(m_mutex);
                if (m_methods.find(name.data()) != m_methods.end())
                    throw ex_method_used("Method already bound.");
            }
            std::unique_lock lock(m_mutex);
            m_methods.emplace(name.data(), method);
        }

        template <typename method_type>
        void add_method(const std::string_view &name, const method_type &method)
        {
            add_method_internal(name, std::function(method));
        }

        template <typename instance_type>
        void add_method(const std::string_view &name, value_t (instance_type::*method)(const params_t &), instance_type &instance)
        {
            add_method(name, std::bind(method, &instance, std::placeholders::_1));
        }

        template <typename instance_type>
        void add_method(const std::string_view &name, value_t (instance_type::*method)(const params_t &) const, instance_type &instance)
        {
            add_method(name, std::bind(method, &instance, std::placeholders::_1));
        }

        template <typename return_type, typename instance_type, typename... params_type>
        void add_method(const std::string_view &name, return_type (instance_type::*method)(params_type...), instance_type &instance)
        {
            std::function<return_type(params_type...)> std_fn = [&instance, method](params_type &&... params) -> return_type {
                return (instance.*method)(std::forward<params_type>(params)...);
            };
            add_method_internal(name, std_fn);
        }

        template <typename return_type, typename instance_type, typename... params_type>
        void add_method(const std::string_view &name, return_type (instance_type::*method)(params_type...) const, instance_type &instance)
        {
            std::function<return_type(params_type...)> std_fn = [&instance, method](params_type &&... params) -> return_type {
                return (instance.*method)(std::forward<params_type>(params)...);
            };
            add_method_internal(name, std_fn);
        }

        const response_t invoke(const request_t &request)
        {
            std::shared_lock lock(m_mutex);
            if (auto iter = m_methods.find(request.get_method()); iter != m_methods.end())
            {
                if (request.is_notification())
                    return response_t(iter->second(request.get_params()));
                else
                    return response_t(iter->second(request.get_params()), request.get_id());
            }

            throw ex_bad_method("Method not bound.");
        }
    };
} // namespace rpc_light