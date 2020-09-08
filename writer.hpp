#pragma once

#include "exceptions.hpp"
#include "value.hpp"
#include "aliases.hpp"
#include "request.hpp"
#include "response.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <string>
#include <vector>

namespace rpc_light
{
    namespace writer
    {
        const rapidjson::Value
        get_id_value(const value_t &id, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc)
        {
            rapidjson::Value id_value;
            std::visit([&](auto &&arg) {
                using type = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<type, null_t>)
                    id_value.SetNull();

                else if constexpr (std::is_same_v<type, int32_t>)
                    id_value.SetInt(id.get_alt<int32_t>(true));

                else if constexpr (std::is_same_v<type, int64_t>)
                    id_value.SetInt64(id.get_alt<int64_t>(true));

                else if constexpr (std::is_same_v<type, std::string>)
                    id_value.SetString(id.get_alt<std::string>(true).c_str(), alloc);

                else
                    throw ex_internal_error("Invalid id type.");
            },
                       id.get_variant());
            return id_value;
        }

        const rapidjson::Value
        get_obj_value(const value_t &obj, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc)
        {
            rapidjson::Value obj_value;
            std::visit([&](auto &&arg) {
                using type = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<type, null_t>)
                    obj_value.SetNull();

                else if constexpr (std::is_same_v<type, array_t>)
                {
                    obj_value.SetArray();
                    auto arr = obj.get_alt<array_t>();
                    for (auto &e : arr)
                        if (auto e_value = get_obj_value(e, alloc); !e_value.IsNull())
                            obj_value.PushBack(e_value, alloc);
                }
                else if constexpr (std::is_same_v<type, bool>)
                    obj_value.SetBool(obj.get_alt<bool>());

                else if constexpr (std::is_same_v<type, double>)
                    obj_value.SetDouble(obj.get_alt<double>());

                else if constexpr (std::is_same_v<type, int32_t>)
                    obj_value.SetInt(obj.get_alt<int32_t>());

                else if constexpr (std::is_same_v<type, int64_t>)
                    obj_value.SetInt64(obj.get_alt<int64_t>());

                else if constexpr (std::is_same_v<type, std::string>)
                    obj_value.SetString(obj.get_alt<std::string>().c_str(), alloc);

                else if constexpr (std::is_same_v<type, struct_t>)
                {
                    obj_value.SetObject();
                    auto strct = obj.get_alt<struct_t>();
                    for (auto &e : strct)
                        obj_value.AddMember(rapidjson::Value(e.first.c_str(), alloc),
                                            (rapidjson::Value)get_obj_value(e.second, alloc), alloc);
                }
                else
                    throw ex_internal_error("Invalid object type.");
            },
                       obj.get_variant());
            return obj_value;
        }

        const std::string
        serialize_batch_request(const std::vector<request_t> &requests)
        {
            rapidjson::Document document;
            document.SetArray();
            auto &alloc = document.GetAllocator();
            for (auto &e : requests)
            {
                rapidjson::Value request;
                request.SetObject();
                request.AddMember(JSON_PROTO, JSON_VER, alloc);
                request.AddMember(JSON_METHOD, rapidjson::Value(e.get_method().c_str(), alloc), alloc);
                if (e.has_params())
                {
                    if (e.has_named_params())
                    {
                        rapidjson::Value params_value = get_obj_value(e.get_params_str(), alloc);
                        if (params_value.IsNull())
                            throw ex_internal_error("Batch params object was null.");

                        request.AddMember(JSON_PARAMS, params_value, alloc);
                    }
                    else
                    {
                        rapidjson::Value params_value = get_obj_value(e.get_params_arr(), alloc);
                        if (params_value.IsNull())
                            throw ex_internal_error("Batch params array was null.");

                        request.AddMember(JSON_PARAMS, params_value, alloc);
                    }
                }

                if (!e.is_notification())
                {
                    auto id_value = get_id_value(e.get_id(), alloc);
                    if (id_value.IsNull())
                        throw ex_internal_error("Batch request was not notification with null id.");

                    request.AddMember(JSON_ID, id_value, alloc);
                }
                document.PushBack(request, alloc);
            }
            rapidjson::StringBuffer strbuf;
            rapidjson::Writer writer(strbuf);
            document.Accept(writer);
            return strbuf.GetString();
        }

        const std::string
        serialize_batch_response(const std::vector<response_t> &responses)
        {
            rapidjson::Document document;
            document.SetArray();
            auto &alloc = document.GetAllocator();
            for (auto &e : responses)
            {
                if (e.is_notification() && !e.has_error())
                    continue;

                rapidjson::Value response;
                response.SetObject();
                response.AddMember(JSON_PROTO, JSON_VER, alloc);
                auto id_value = get_id_value(e.get_id(), alloc);
                if (e.has_error())
                {
                    auto code_value = get_obj_value(e.get_code(), alloc);
                    if (code_value.IsNull())
                        throw ex_internal_error("Batch null code value.");

                    auto message_value = get_obj_value(e.get_message(), alloc);
                    if (message_value.IsNull())
                        throw ex_internal_error("Batch null message value.");

                    auto data_value = get_obj_value(e.get_data(), alloc);
                    rapidjson::Value error_obj;
                    error_obj.SetObject();
                    error_obj.AddMember(JSON_CODE, code_value, alloc);
                    error_obj.AddMember(JSON_MESSAGE, message_value, alloc);
                    if (!data_value.IsNull())
                        error_obj.AddMember(JSON_DATA, data_value, alloc);

                    response.AddMember(JSON_ERROR, error_obj, alloc);
                }
                else
                {
                    if (id_value.IsNull())
                        throw ex_internal_error("Batch response was not notification with null id.");

                    auto result_value = get_obj_value(e.get_value(), alloc);
                    response.AddMember(JSON_RESULT, result_value, alloc);
                }
                response.AddMember(JSON_ID, id_value, alloc);
                document.PushBack(response, alloc);
            }
            rapidjson::StringBuffer strbuf;
            rapidjson::Writer writer(strbuf);
            document.Accept(writer);
            return strbuf.GetString();
        }

        const std::string
        serialize_request(const request_t &request)
        {
            rapidjson::Document document;
            document.SetObject();
            auto &alloc = document.GetAllocator();
            document.AddMember(JSON_PROTO, JSON_VER, alloc);
            document.AddMember(JSON_METHOD, rapidjson::Value(request.get_method().c_str(), alloc), alloc);
            if (request.has_params())
            {
                if (request.has_named_params())
                {
                    rapidjson::Value params_value = get_obj_value(request.get_params_str(), alloc);
                    if (params_value.IsNull())
                        throw ex_internal_error("Params object was null.");

                    document.AddMember(JSON_PARAMS, params_value, alloc);
                }
                else
                {
                    rapidjson::Value params_value = get_obj_value(request.get_params_arr(), alloc);
                    if (params_value.IsNull())
                        throw ex_internal_error("Params array was null.");

                    document.AddMember(JSON_PARAMS, params_value, alloc);
                }
            }

            if (!request.is_notification())
            {
                auto id_value = get_id_value(request.get_id(), alloc);
                if (id_value.IsNull())
                    throw ex_internal_error("Request was not notification with null id.");

                document.AddMember(JSON_ID, id_value, alloc);
            }
            rapidjson::StringBuffer strbuf;
            rapidjson::Writer writer(strbuf);
            document.Accept(writer);
            return strbuf.GetString();
        }

        const std::string
        serialize_response(const response_t &response)
        {
            if (response.is_notification() && !response.has_error())
                return "";

            rapidjson::Document document;
            document.SetObject();
            auto &alloc = document.GetAllocator();
            document.AddMember(JSON_PROTO, JSON_VER, alloc);
            auto id_value = get_id_value(response.get_id(), alloc);
            if (response.has_error())
            {
                auto code_value = get_obj_value(response.get_code(), alloc);
                if (code_value.IsNull())
                    throw ex_internal_error("Null code value.");

                auto message_value = get_obj_value(response.get_message(), alloc);
                if (message_value.IsNull())
                    throw ex_internal_error("Null message value.");

                auto data_value = get_obj_value(response.get_data(), alloc);
                rapidjson::Value error_obj;
                error_obj.SetObject();
                error_obj.AddMember(JSON_CODE, code_value, alloc);
                error_obj.AddMember(JSON_MESSAGE, message_value, alloc);
                if (!data_value.IsNull())
                    error_obj.AddMember(JSON_DATA, data_value, alloc);

                document.AddMember(JSON_ERROR, error_obj, alloc);
            }
            else
            {
                if (id_value.IsNull())
                    throw ex_internal_error("Response was not notification with null id.");

                auto result_value = get_obj_value(response.get_value(), alloc);
                document.AddMember(JSON_RESULT, result_value, alloc);
            }
            document.AddMember(JSON_ID, id_value, alloc);
            rapidjson::StringBuffer strbuf;
            rapidjson::Writer writer(strbuf);
            document.Accept(writer);
            return strbuf.GetString();
        }
    }; // namespace writer
} // namespace rpc_light