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

User Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<User>);

userver::formats::json::Value Serialize(const UserInfo& data,
                               userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace messenger::user_management