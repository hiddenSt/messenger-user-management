#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json.hpp>

namespace messenger::user_management {

struct User {
  std::string name;
  std::string first_name;
  std::string last_name;
  std::string email;
  std::string password;
};

struct UserInfo {
  std::int32_t id;
  std::string username;
  std::string first_name;
  std::string last_name;
  std::string email;
};

namespace json = userver::formats::json;
namespace formats = userver::formats;

User Parse(const json::Value& json, formats::parse::To<User>);

json::Value Serialize(const UserInfo& data, formats::serialize::To<json::Value>);

}  // namespace messenger::user_management