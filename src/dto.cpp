#include <dto.hpp>

#include <userver/formats/json/value_builder.hpp>

namespace messenger::user_management {

User Parse(const json::Value& json, formats::parse::To<User>) {
  return User{
      json["username"].As<std::string>(), json["first_name"].As<std::string>(),
      json["last_name"].As<std::string>(), json["email"].As<std::string>(),
      json["password"].As<std::string>()};
}

json::Value Serialize(const UserInfo& data,
                      formats::serialize::To<json::Value>) {
  json::ValueBuilder builder;
  builder["id"] = data.id;
  builder["email"] = data.email;
  builder["username"] = data.username;
  builder["first_name"] = data.first_name;
  builder["last_name"] = data.last_name;

  return builder.ExtractValue();
}

}  // namespace messenger::user_management