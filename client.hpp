#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "request.hpp"
#include "response.hpp"
#include "result.hpp"

#include <string>
#include <future>
#include <vector>

namespace rpc_light
{
    class client_t
    {

        const response_t
        handle_error(const std::exception_ptr &e_ptr, const value_t &id = null_t()) const
        {
            try
            {
                std::rethrow_exception(e_ptr);
            }
            catch (const ex_bad_params &e)
            {
                return response_t(-32602, e.what(), id, e.data());
            }
            catch (const ex_internal_error &e)
            {
                return response_t(-32603, e.what(), id, e.data());
            }
            catch (const ex_bad_request &e)
            {
                return response_t(-32600, e.what(), id, e.data());
            }
            catch (const ex_method_used &e)
            {
                return response_t(-32000, e.what(), id, e.data());
            }
            catch (const ex_bad_method &e)
            {
                return response_t(-32601, e.what(), id, e.data());
            }
            catch (const ex_parse_error &e)
            {
                return response_t(-32700, e.what(), id, e.data());
            }
            catch (const std::exception &e)
            {
                return response_t(-32098, "Unknown error occured.", id, e.what());
            }
            catch (...)
            {
                return response_t(-32099, "Unknown error occured.", id, "");
            }
        }

    public:
        const inline std::string
        create_request(const std::string_view &method_name, const value_t &id) const
        {
            return writer::serialize_request(request_t(method_name, id));
        }
        const inline std::string
        create_request(const std::string_view &method_name, const value_t &id, const std::initializer_list<value_t> &params) const
        {
            return writer::serialize_request(request_t(method_name, array_t{params}, id));
        }

        const inline std::string
        create_request(const std::string_view &method_name, const value_t &id, const std::initializer_list<std::pair<const std::string, value_t>> &params) const
        {
            return writer::serialize_request(request_t(method_name, struct_t{params}, id));
        }

        const inline std::string
        create_request(const std::string_view &method_name) const
        {
            return writer::serialize_request(request_t(method_name));
        }

        const inline std::string
        create_request(const std::string_view &method_name, const std::initializer_list<value_t> &params) const
        {
            return writer::serialize_request(request_t(method_name, array_t{params}));
        }

        const inline std::string
        create_request(const std::string_view &method_name, const std::initializer_list<std::pair<const std::string, value_t>> &params) const
        {
            return writer::serialize_request(request_t(method_name, struct_t{params}));
        }

        template <typename... params_type>
        const inline std::string
        create_batch(const params_type &... params) const
        {
            return writer::serialize_batch_request({{reader::deserialize_request(params)...}});
        }

        std::future<result_t> handle_response(const std::string response_string)
        {
            return std::async(
                std::launch::async, [=](std::string &&resp_str) {
                    try
                    {
                        if (auto batch = reader::get_batch(resp_str); !batch.empty())
                        {
                            std::vector<response_t> responses;
                            std::vector<std::future<result_t>> futures;
                            for (auto &e : batch)
                                futures.push_back(handle_response(e));

                            responses.reserve(futures.size());
                            bool has_error = false;
                            for (auto &e : futures)
                            {
                                auto result = e.get();
                                if (result.has_error())
                                    has_error = true;

                                responses.push_back(result.get_response());
                            }
                            return result_t(responses, has_error);
                        }
                        auto response = reader::deserialize_response(resp_str);
                        return result_t(response);
                    }
                    catch (...)
                    {
                        auto error = handle_error(std::current_exception());
                        return result_t(error, true);
                    }
                },
                response_string);
        }
    };
} // namespace rpc_light