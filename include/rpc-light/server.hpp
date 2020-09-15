#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "dispatcher.hpp"
#include "result.hpp"
#include "response.hpp"

#include <string>
#include <future>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace rpc_light
{
    class server_t
    {
        std::mutex m_mutex;
        dispatcher_t m_dispatcher;
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
                return response_t(-32099, "Unknown error occured.", id);
            }
        }

        const bool is_working() const
        {
            return (m_worker.valid() && m_worker.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout);
        }

        void start_worker()
        {
            m_worker = std::async(std::launch::async, &server_t::worker_proc, this);
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

        result_t get_result(const std::string request_string)
        {
            try
            {
                if (auto batch = reader::get_batch(request_string); !batch.empty())
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
                    return result_t(responses, writer::serialize_batch_response(responses), has_error);
                }
                auto request = reader::deserialize_request(request_string);
                try
                {
                    auto response = m_dispatcher.invoke(request);
                    return result_t(response, writer::serialize_response(response));
                }
                catch (...)
                {
                    auto error = handle_error(std::current_exception(), request.get_id());
                    return result_t(error, writer::serialize_response(error), true);
                }
            }
            catch (...)
            {
                auto error = handle_error(std::current_exception());
                return result_t(error, writer::serialize_response(error), true);
            }
        }

    public:
        auto handle_request(const std::string &request_string)
        {
            std::future<const result_t> result;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!is_working())
                    start_worker();

                m_queue.push_back({request_string, std::promise<const result_t>()});
                result = m_queue.back().second.get_future();
            }
            event.notify_all();
            return result;
        }

        inline dispatcher_t &get_dispatcher()
        {
            return m_dispatcher;
        }
    };
} // namespace rpc_light