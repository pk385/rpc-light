#pragma once

#include "exceptions.hpp"
#include "aliases.hpp"
#include "value.hpp"
#include "request.hpp"
#include "response.hpp"
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"

#include <string>
#include <vector>

namespace rpc_light
{
    namespace reader
    {
        const value_t get_id_obj(const rapidjson::Value &id)
        {
            if (id.IsString())
                return std::string(id.GetString());

            else if (id.IsInt())
                return id.GetInt();

            else if (id.IsInt64())
                return id.GetInt64();

            else if (id.IsNull())
                return null_t();

            throw ex_bad_request("Invalid id type.");
        }

        const value_t get_value_obj(const rapidjson::Value &value)
        {
            switch (value.GetType())
            {
            case rapidjson::kNullType:
                return null_t();

            case rapidjson::kFalseType:
                return value.GetBool();

            case rapidjson::kTrueType:
                return value.GetBool();

            case rapidjson::kObjectType:
            {
                struct_t data;
                for (auto &e : value.GetObject())
                    data.emplace(std::string_view(e.name.GetString()), get_value_obj(e.value));

                return data;
            }
            case rapidjson::kArrayType:
            {
                array_t array;
                array.reserve(value.Size());
                for (auto &e : value.GetArray())
                    array.emplace_back(get_value_obj(e));

                return array;
            }
            case rapidjson::kStringType:
                return std::string(value.GetString());

            case rapidjson::kNumberType:
            {
                if (value.IsDouble())
                    return value.GetDouble();

                else if (value.IsInt())
                    return value.GetInt();

                else if (value.IsUint())
                    return (int64_t)value.GetUint();

                else if (value.IsInt64())
                    return value.GetInt64();

                else if (value.IsUint64())
                    return (double)value.GetUint64();
            }
            }
            throw ex_bad_request("Invalid object type.");
        }

        const std::vector<std::string> get_batch(const std::string_view &str)
        {
            std::vector<std::string> batch;
            rapidjson::Document document;
            document.Parse(str.data());
            if (document.HasParseError())
                throw ex_parse_error("Batch parse error.");

            if (document.IsArray())
            {
                auto arr = document.GetArray();
                batch.reserve(arr.Size());
                for (auto &e : arr)
                {
                    rapidjson::StringBuffer strbuf;
                    rapidjson::Writer writer(strbuf);
                    e.Accept(writer);
                    batch.push_back(strbuf.GetString());
                }
            }
            return batch;
        }

        const request_t deserialize_request(const std::string_view &request_string)
        {
            rapidjson::Document document;
            document.Parse(request_string.data());
            if (document.HasParseError())
                throw ex_parse_error("Request parse error.");
            if (!document.IsObject())
                throw ex_bad_request("Request was not an object.");

            auto member_end = document.MemberEnd();
            auto jrpc_version = document.FindMember(JSON_PROTO);
            auto method = document.FindMember(JSON_METHOD);
            auto json_params = document.FindMember(JSON_PARAMS);
            auto id = document.FindMember(JSON_ID);

            if (jrpc_version == member_end || !jrpc_version->value.IsString())
                throw ex_bad_request("Invalid protocol.");

            if (std::string_view(jrpc_version->value.GetString()) != JSON_VER)
                throw ex_bad_request("Invalid protocol version.");

            if (method == member_end || !method->value.IsString())
                throw ex_bad_request("Invalid method value.");

            if (json_params != member_end)
            {
                if (json_params->value.IsArray())
                {
                    if (id == member_end)
                        return request_t(method->value.GetString(), get_value_obj(json_params->value).get_value<array_t>());

                    return request_t(method->value.GetString(), get_value_obj(json_params->value).get_value<array_t>(), get_id_obj(id->value));
                }
                else if (json_params->value.IsObject())
                {
                    if (id == member_end)
                        return request_t(method->value.GetString(), get_value_obj(json_params->value).get_value<struct_t>());

                    return request_t(method->value.GetString(), get_value_obj(json_params->value).get_value<struct_t>(), get_id_obj(id->value));
                }
                else
                {
                    throw ex_bad_request();
                }
            }

            if (id == member_end)
                return request_t(method->value.GetString());

            return request_t(method->value.GetString(), get_id_obj(id->value));
        }

        const response_t deserialize_response(const std::string_view &response_string)
        {
            rapidjson::Document document;
            document.Parse(response_string.data());
            if (document.HasParseError())
                throw ex_parse_error("Response parse error.");

            if (!document.IsObject())
                throw ex_bad_request("Response was not an object.");

            auto member_end = document.MemberEnd();
            auto jrpc_version = document.FindMember(JSON_PROTO);
            auto id = document.FindMember(JSON_ID);
            auto result = document.FindMember(JSON_RESULT);
            auto error = document.FindMember(JSON_ERROR);

            if (jrpc_version == member_end || !jrpc_version->value.IsString())
                throw ex_bad_request("Invalid protocol.");

            if (std::string_view(jrpc_version->value.GetString()) != JSON_VER)
                throw ex_bad_request("Invalid protocol version.");

            if (id == member_end)
                throw ex_bad_request("Missing response id.");

            if (result != member_end)
            {
                if (error != member_end)
                    throw ex_bad_request("Non-exclusive result.");

                return response_t(get_value_obj(result->value), get_id_obj(id->value));
            }
            else if (error != member_end)
            {
                if (result != member_end)
                    throw ex_bad_request("Non-exclusive result.");

                if (!error->value.IsObject())
                    throw ex_bad_request("Error was not an object.");

                auto code = error->value.FindMember(JSON_CODE);
                if (code == member_end || !code->value.IsInt())
                    throw ex_bad_request("Invalid error code value.");

                auto message = error->value.FindMember(JSON_MESSAGE);
                if (message == member_end || !message->value.IsString())
                    throw ex_bad_request("Invalid error message value.");

                auto data = error->value.FindMember(JSON_DATA);
                if (data != member_end)
                    return response_t(code->value.GetInt(), message->value.GetString(),
                                      get_id_obj(id->value), get_value_obj(data->value));

                return response_t(code->value.GetInt(), message->value.GetString(),
                                  get_id_obj(id->value));
            }
            else
                throw ex_bad_request("Non-inclusive result.");
        }
    }; // namespace reader
} // namespace rpc_light