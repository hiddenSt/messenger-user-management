#include "user_management_handler.hpp"

#include "dto.hpp"

#include <cstdint>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/utils/assert.hpp>

namespace messenger::user_management {

namespace {

class CreateUserHandler final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-create-user";

  CreateUserHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    auto user = json.As<User>();

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO messenger_schema.users(username, first_name, last_name, "
        "email) VALUES ($1, $2, $3, $4)",
        user.name, user.first_name, user.last_name, user.email);

    if (result.RowsAffected() == 0) {
      throw userver::server::handlers::ClientError();
    }

    auto id_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, username, first_name, last_name, email FROM messenger_schema.users WHERE username = $1 AND email = $2",
        user.name, user.email);

    if (id_result.IsEmpty()) {
      throw userver::server::handlers::InternalServerError();
    }

    auto result_set = id_result.AsSetOf<UserInfo>(userver::storages::postgres::kRowTag);
    UserInfo user_info = result_set[0];
    userver::formats::json::ValueBuilder response;
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    response["user"] = user_info;
    
    return response.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class GetUserHandler final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-get-user";

  GetUserHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<userver::components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value&,
      userver::server::request::RequestContext&) const override {

    std::int32_t id = std::stol(request.GetPathArg("id"));
    auto query_result = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
    "SELECT id, username, first_name, last_name, email FROM messenger_schema.users WHERE id = $1", id);

    if (query_result.RowsAffected() == 0) {
      throw userver::server::handlers::ResourceNotFound();
    }

    auto result_set = query_result.AsSetOf<UserInfo>(userver::storages::postgres::kRowTag);
    UserInfo user_info = result_set[0];
    userver::formats::json::ValueBuilder response;
    response["user"] = user_info;

    return response.ExtractValue();
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class DeleteUserHandler final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-delete-user";

  DeleteUserHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<userver::components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http ::HttpRequest& request,
      const userver::formats::json::Value&,
      userver::server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));

    auto query_result = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
    "DELETE FROM messenger_schema.users WHERE id = $1", id);

    if (query_result.RowsAffected() == 0) {
      throw userver::server::handlers::ResourceNotFound();
    }

    request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
    
    return {};
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class PutUserHandler final
    : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-put-user";

  PutUserHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        pg_cluster_(context
                        .FindComponent<userver::components::Postgres>(
                            "messenger-user-management")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http ::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    std::int32_t id = std::stol(request.GetPathArg("id"));
    User user = json.As<User>();

    pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster, "", id);
    
    return {};
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendUserManagement(userver::components::ComponentList& component_list) {
  component_list.Append<CreateUserHandler>()
      .Append<GetUserHandler>()
      .Append<DeleteUserHandler>()
      .Append<PutUserHandler>()
      .Append<userver::components::Postgres>("messenger-user-management")
      .Append<userver::clients::dns::Component>();
}

}  // namespace messenger::user_management
