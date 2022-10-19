#pragma once

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

User Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<User>);

}  // namespace messenger::user_management