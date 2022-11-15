#include <user_management_handler.hpp>

#include <cstdint>
#include <string_view>
#include <vector>
#include <optional>
#include <string>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>

#include <dto.hpp>
#include <user_events_component.hpp>

namespace messenger::user_management {

namespace server = userver::server;
namespace json = userver::formats::json;
namespace postgres = userver::storages::postgres;

namespace {

std::optional<std::string> ValidatePostBody(const json::Value& body) {
  if (!body.HasMember("username")) {
    return "Body must contain string field 'username'.";
  }

  if (!body.HasMember("first_name")) {
    return "Body must contain string field 'first_name'.";
  }

  if (!body.HasMember("last_name")) {
    return "Body must contain string field 'last_name'.";
  }

  if (!body.HasMember("email")) {
    return "Body must contain string field 'email'.";
  }

  if (!body.HasMember("password")) {
    return "Body must contain string field 'password'";
  }

  if (!body["username"].IsString()) {
    return "Field 'username' must be a string.";
  }

  if (!body["first_name"].IsString()) {
    return "Field 'first_name' must be a string.";
  }

  if (!body["last_name"].IsString()) {
    return "Field 'last_name' must be a string.";
  }

  if (!body["email"].IsString()) {
    return "Field 'email' must be a string.";
  }

  if (!body["password"].IsString()) {
    return "Field 'password' must be a string.";
  }

  return std::nullopt;
}

class CreateUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-create-user";

  CreateUserHandler(const components::ComponentConfig& config, const components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        pg_cluster_(component_context.FindComponent<components::Postgres>("messenger-user-management").GetCluster()),
        user_events_client_(component_context.FindComponent<UserEventsComponent>()) {}

  json::Value HandleRequestJsonThrow(const server::http::HttpRequest& request, const json::Value& json,
                                     server::request::RequestContext&) const override {
    auto error_or_nullopt = ValidatePostBody(json);
    json::ValueBuilder response_body;

    if (error_or_nullopt.has_value()) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      response_body["error"] = error_or_nullopt.value();

      return response_body.ExtractValue();
    }

    auto user = json.As<User>();

    auto result = pg_cluster_->Execute(postgres::ClusterHostType::kMaster,
                                       "INSERT INTO messenger_schema.user(username, first_name, last_name, "
                                       "email) VALUES ($1, $2, $3, $4) ON CONFLICT DO NOTHING",
                                       user.username, user.first_name, user.last_name, user.email);

    if (result.RowsAffected() == 0) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      response_body["error"] = "User with given nickname and email already exists.";
      
      return response_body.ExtractValue();
    }

    auto id_result = pg_cluster_->Execute(postgres::ClusterHostType::kMaster,
                                          "SELECT id, username, first_name, last_name, email FROM "
                                          "messenger_schema.user WHERE username = $1 AND email = $2",
                                          user.username, user.email);

    if (id_result.IsEmpty()) {
      throw server::handlers::InternalServerError();
    }

    auto result_set = id_result.AsSetOf<User>(postgres::kRowTag);
    User user_info = result_set[0];
    json::ValueBuilder response;
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    response["user"] = user_info;
    user_events_client_.NotifyNewUserIsCreated(user_info.id);

    return response.ExtractValue();
  }

 private:
  postgres::ClusterPtr pg_cluster_;
  UserEventsComponent& user_events_client_;
};

class GetUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-get-user";

  GetUserHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context.FindComponent<components::Postgres>("messenger-user-management").GetCluster()) {}

  json::Value HandleRequestJsonThrow(const server::http::HttpRequest& request, const json::Value&,
                                     server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));
    auto query_result = pg_cluster_->Execute(postgres::ClusterHostType::kMaster,
                                             "SELECT id, username, first_name, last_name, email FROM "
                                             "messenger_schema.user WHERE id = $1",
                                             id);

    if (query_result.RowsAffected() == 0) {
      throw server::handlers::ResourceNotFound();
    }

    auto result_set = query_result.AsSetOf<User>(postgres::kRowTag);
    User user = result_set[0];
    json::ValueBuilder response;
    response["user"] = user;

    return response.ExtractValue();
  }

 private:
  postgres::ClusterPtr pg_cluster_;
};

class DeleteUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-delete-user";

  DeleteUserHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context.FindComponent<components::Postgres>("messenger-user-management").GetCluster()),
        user_events_client_(context.FindComponent<UserEventsComponent>()) {}

  json::Value HandleRequestJsonThrow(const server::http ::HttpRequest& request, const json::Value&,
                                     server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));

    auto query_result =
        pg_cluster_->Execute(postgres::ClusterHostType::kMaster, "DELETE FROM messenger_schema.user WHERE id = $1", id);

    if (query_result.RowsAffected() == 0) {
      throw server::handlers::ResourceNotFound();
    }

    request.SetResponseStatus(server::http::HttpStatus::kNoContent);
    user_events_client_.NotifyUserIsDeleted(id);

    return {};
  }

 private:
  postgres::ClusterPtr pg_cluster_;
  UserEventsComponent& user_events_client_;
};

}  // namespace

void AppendUserManagement(components::ComponentList& component_list) {
  component_list.Append<CreateUserHandler>()
      .Append<GetUserHandler>()
      .Append<DeleteUserHandler>()
      .Append<components::Postgres>("messenger-user-management")
      .Append<userver::clients::dns::Component>()
      .Append<components::Secdist>()
      .Append<UserEventsComponent>();
}

}  // namespace messenger::user_management
