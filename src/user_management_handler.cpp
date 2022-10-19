#include "user_management_handler.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

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
#include <userver/utils/assert.hpp>

#include <dto.hpp>
#include <user_events_component.hpp>

namespace messenger::user_management {

namespace server = userver::server;
namespace json = userver::formats::json;
namespace postgres = userver::storages::postgres;

namespace {

class CreateUserHandler final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-create-user";

  CreateUserHandler(const components::ComponentConfig& config,
                    const components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        pg_cluster_(component_context
                        .FindComponent<components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()),
        user_events_client_(
            component_context.FindComponent<UserEventsComponent>()) {}

  json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request, const json::Value& json,
      server::request::RequestContext&) const override {
    auto user = json.As<User>();

    auto result = pg_cluster_->Execute(
        postgres::ClusterHostType::kMaster,
        "INSERT INTO messenger_schema.users(username, first_name, last_name, "
        "email) VALUES ($1, $2, $3, $4)",
        user.name, user.first_name, user.last_name, user.email);

    if (result.RowsAffected() == 0) {
      throw server::handlers::ClientError();
    }

    auto id_result = pg_cluster_->Execute(
        postgres::ClusterHostType::kMaster,
        "SELECT id, username, first_name, last_name, email FROM "
        "messenger_schema.users WHERE username = $1 AND email = $2",
        user.name, user.email);

    if (id_result.IsEmpty()) {
      throw server::handlers::InternalServerError();
    }

    auto result_set = id_result.AsSetOf<UserInfo>(postgres::kRowTag);
    UserInfo user_info = result_set[0];
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

  GetUserHandler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request, const json::Value&,
      server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));
    auto query_result = pg_cluster_->Execute(
        postgres::ClusterHostType::kMaster,
        "SELECT id, username, first_name, last_name, email FROM "
        "messenger_schema.users WHERE id = $1",
        id);

    if (query_result.RowsAffected() == 0) {
      throw server::handlers::ResourceNotFound();
    }

    auto result_set = query_result.AsSetOf<UserInfo>(postgres::kRowTag);
    UserInfo user_info = result_set[0];
    json::ValueBuilder response;
    response["user"] = user_info;

    return response.ExtractValue();
  }

 private:
  postgres::ClusterPtr pg_cluster_;
};

class DeleteUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-delete-user";

  DeleteUserHandler(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()),
        user_events_client_(context.FindComponent<UserEventsComponent>()) {}

  json::Value HandleRequestJsonThrow(
      const server::http ::HttpRequest& request, const json::Value&,
      server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));

    auto query_result = pg_cluster_->Execute(
        postgres::ClusterHostType::kMaster,
        "DELETE FROM messenger_schema.users WHERE id = $1", id);

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

class PutUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-put-user";

  PutUserHandler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  json::Value HandleRequestJsonThrow(
      const server::http ::HttpRequest& request, const json::Value& json,
      server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));
    User user = json.As<User>();

    pg_cluster_->Execute(postgres::ClusterHostType::kMaster, "", id);

    return {};
  }

 private:
  postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendUserManagement(components::ComponentList& component_list) {
  component_list.Append<CreateUserHandler>()
      .Append<GetUserHandler>()
      .Append<DeleteUserHandler>()
      .Append<PutUserHandler>()
      .Append<components::Postgres>("messenger-user-management")
      .Append<userver::clients::dns::Component>()
      .Append<components::Secdist>()
      .Append<UserEventsComponent>();
}

}  // namespace messenger::user_management
