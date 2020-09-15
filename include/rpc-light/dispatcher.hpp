#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"
#include "response.hpp"
#include "request.hpp"

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>

namespace rpc_light
{

    class dispatcher_t
    {
        std::unordered_map<std::string, method_t> m_methods;
        std::unordered_map<std::string, param_map_t> m_mappings;

        const array_t struct_params_to_arr(const std::string_view &name, const struct_t &params)
        {
            if (auto method_iter = m_mappings.find(name.data()); method_iter != m_mappings.end())
            {
                array_t arr_params;
                auto params_size = params.size();
                arr_params.reserve(params_size);
                for (auto i = 0; i < params_size; i++)
                {
                    if (auto index_iter = method_iter->second.find(i); index_iter != method_iter->second.end())
                    {
                        if (auto params_iter = params.find(index_iter->second); params_iter != params.end())
                        {
                            arr_params.emplace_back(params_iter->second);
                        }
                        else
                        {
                            throw ex_internal_error("Param not found.");
                        }
                    }
                    else
                    {
                        throw ex_internal_error("Index not found.");
                    }
                }
                return arr_params;
            }
            throw ex_internal_error("Params mapping not found.");
        }

        template <typename return_type, typename... params_type>
        void add_method_internal(const std::string_view &name, const std::function<return_type(params_type...)> &method)
        {
            add_method_internal(name, method, std::index_sequence_for<params_type...>());
        }

        template <typename return_type, typename... params_type, std::size_t... index>
        void add_method_internal(const std::string_view &name, const std::function<return_type(params_type...)> &method, const std::index_sequence<index...>)
        {
            method_t expr = [method](const array_t &params) -> value_t {
                constexpr auto params_size = sizeof...(params_type);
                if (params_size != params.size())
                    throw ex_bad_params("Params length mismatch.");

                try
                {
                    if constexpr (!std::is_void_v<return_type>)
                    {
                        if constexpr (params_size > 0)
                            return value_t(method(params[index].get_value<std::decay_t<params_type>>()...));

                        else
                            return value_t(method());
                    }
                    else
                    {
                        if constexpr (params_size > 0)
                            method(params[index].get_value<std::decay_t<params_type>>()...);

                        else
                            method();

                        return null_t();
                    }
                }
                catch (...)
                {
                    throw ex_bad_params("Invalid param types.");
                }
            };
            add_method(name, expr);
        }

    public:
        dispatcher_t() {}

        template <typename method_type>
        void add_method(const std::string_view &name, const method_type &method)
        {
            add_method_internal(name, std::function(method));
        }

        template <typename instance_type>
        void add_method(const std::string_view &name, value_t (instance_type::*method)(const array_t &), instance_type &instance)
        {
            add_method(name, std::bind(method, &instance, std::placeholders::_1));
        }

        template <typename instance_type>
        void add_method(const std::string_view &name, value_t (instance_type::*method)(const array_t &) const, instance_type &instance)
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

        void add_method(const std::string_view &name, const method_t &method)
        {
            if (m_methods.find(name.data()) != m_methods.end())
                throw ex_method_used("Method already bound.");

            m_methods.emplace(name, method);
        }

        void add_param_mapping(const std::string_view &name, const param_map_t &mapping)
        {
            if (m_mappings.find(name.data()) != m_mappings.end())
                throw ex_method_used("Method params mapping already bound.");

            m_mappings.emplace(name, mapping);
        }

        const response_t invoke(const request_t &request)
        {
            if (auto iter = m_methods.find(request.get_method()); iter != m_methods.end())
            {
                if (request.is_notification())
                {
                    if (!request.has_params())
                        return response_t(iter->second(array_t()));

                    if (!request.has_named_params())
                        return response_t(iter->second(request.get_params_arr()));

                    return response_t(iter->second(struct_params_to_arr(request.get_method(), request.get_params_str())));
                }
                else
                {
                    if (!request.has_params())
                        return response_t(iter->second(array_t()), request.get_id());

                    if (!request.has_named_params())
                        return response_t(iter->second(request.get_params_arr()), request.get_id());

                    return response_t(iter->second(struct_params_to_arr(request.get_method(), request.get_params_str())), request.get_id());
                }
            }

            throw ex_bad_method("Method not bound.");
        }
    };
} // namespace rpc_light