#pragma once

#include "exceptions.hpp"
#include "value.hpp"
#include "aliases.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "dispatcher.hpp"
#include "result.hpp"
#include "response.hpp"

#include <string>
#include <future>
#include <vector>

namespace rpc_light
{
    class server_t
    {
        reader_t m_reader;
        writer_t m_writer;
        dispatcher_t m_dispatcher;

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
                return response_t(-32099, "Unknown error occured.", id);
            }
        }

    public:
        inline dispatcher_t &get_dispatcher()
        {
            return m_dispatcher;
        }

        std::future<result_t> handle_request(const std::string request_string)
        {
            return std::async(
                std::launch::async, [=](const std::string req_str) {
                    try
                    {
                        if (auto batch = m_reader.get_batch(req_str); !batch.empty())
                        {
                            std::vector<response_t> responses;
                            std::vector<std::future<result_t>> futures;
                            auto requests = m_reader.get_batch(req_str);
                            for (auto &e : batch)
                                futures.push_back(handle_request(e));

                            bool has_error = false;
                            for (auto &e : futures)
                            {
                                auto result = e.get();
                                if (result.has_error())
                                    has_error = true;
                                responses.push_back(result.get_response());
                            }
                            return result_t(responses, m_writer.serialize_batch_response(responses), has_error);
                        }
                        auto request = m_reader.deserialize_request(req_str);
                        try
                        {
                            auto response = m_dispatcher.invoke(request);
                            return result_t(response, m_writer.serialize_response(response));
                        }
                        catch (...)
                        {
                            auto error = handle_error(std::current_exception(), request.get_id());
                            return result_t(error, m_writer.serialize_response(error), true);
                        }
                    }
                    catch (...)
                    {
                        auto error = handle_error(std::current_exception());
                        return result_t(error, m_writer.serialize_response(error), true);
                    }
                },
                request_string);
        }
    };
} // namespace rpc_light