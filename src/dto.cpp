#include "dto.hpp"

namespace messenger::user_management {

User Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<User>) {
  return User{
      json["username"].As<std::string>(), json["first_name"].As<std::string>(),
      json["last_name"].As<std::string>(), json["email"].As<std::string>(),
      json["password"].As<std::string>()};
}

}  // namespace messenger::user_management