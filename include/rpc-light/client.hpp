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
#include <mutex>
#include <condition_variable>

namespace rpc_light
{
    class client_t
    {
        std::mutex m_mutex;
        std::future<void> m_worker;
        std::condition_variable event;
        std::vector<std::pair<const std::string, std::promise<const result_t>>> m_queue;

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

        const bool is_working() const
        {
            return (m_worker.valid() && m_worker.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
        }

        void start_worker()
        {
            m_worker = std::async(std::launch::async, &client_t::worker_proc, this);
        }

        void worker_proc()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!event.wait_for(lock, std::chrono::seconds(5), [&] { return !m_queue.empty(); }))
                    return;

                for (auto &e : m_queue)
                {
                    e.second.set_value(get_result(e.first));
                }
                m_queue.clear();
            }
        }

        result_t get_result(const std::string &response_string)
        {
            try
            {
                if (auto batch = reader::get_batch(response_string); !batch.empty())
                {
                    std::vector<response_t> responses;
                    bool has_error = false;
                    for (auto &e : batch)
                    {
                        auto result = get_result(e);
                        if (result.has_error())
                            has_error = true;

                        responses.push_back(result.get_response());
                    }
                    return result_t(responses, has_error);
                }
                auto response = reader::deserialize_response(response_string);
                return result_t(response);
            }
            catch (...)
            {
                auto error = handle_error(std::current_exception());
                return result_t(error, true);
            }
        }

    public:
        auto handle_response(const std::string response_string)
        {
            std::future<const result_t> result;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!is_working())
                    start_worker();

                m_queue.push_back({response_string, std::promise<const result_t>()});
                result = m_queue.back().second.get_future();
            }
            event.notify_all();
            return result;
        }

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
    };
} // namespace rpc_light