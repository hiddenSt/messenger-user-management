#include "user_management_handler.hpp"

#include "dto.hpp"
#include "userver/server/http/http_status.hpp"

#include <cstdint>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>
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

    userver::formats::json::ValueBuilder response;
    // response["id"] = idResult[0][0].As<std::uint64_t>();
    response["username"] = id_result[0][1].As<std::string>();
    response["first_name"] = id_result[0][2].As<std::string>();
    response["last_name"] = id_result[0][3].As<std::string>();
    response["email"] = id_result[0][4].As<std::string>();
    request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
    
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
    std::string id_str = request.GetPathArg("id");
    
    auto user_info_request = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
    "SELECT id, email, username, first_name, last_name FROM messenger_schema.users WHERE id = $1", id_str);

    if (user_info_request.RowsAffected() == 0) {
      throw userver::server::handlers::ResourceNotFound();
    }
    
    userver::formats::json::ValueBuilder response;
    response["id"] = user_info_request[0][0].As<std::string>();
    response["email"] = user_info_request[0][1].As<std::string>();
    response["username"] = user_info_request[0][2].As<std::string>();
    response["first_name"] = user_info_request[0][3].As<std::string>();
    response["last_name"] = user_info_request[0][4].As<std::string>();

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
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
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
