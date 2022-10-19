#include "dto.hpp"
#include "userver/formats/json/value_builder.hpp"

namespace messenger::user_management {

User Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<User>) {
  return User{
      json["username"].As<std::string>(), json["first_name"].As<std::string>(),
      json["last_name"].As<std::string>(), json["email"].As<std::string>(),
      json["password"].As<std::string>()};
}

userver::formats::json::Value Serialize(const UserInfo& data,
                               userver::formats::serialize::To<userver::formats::json::Value>)
{
    userver::formats::json::ValueBuilder builder;
    builder["id"] = data.id;
    builder["email"] = data.email;
    builder["username"] = data.username;
    builder["first_name"] = data.first_name;
    builder["last_name"] = data.last_name;

    return builder.ExtractValue();
}

}  // namespace messenger::user_management