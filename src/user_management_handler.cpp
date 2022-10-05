#include "user_management_handler.hpp"

#include "user_events_component.hpp"

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
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
                            "postgres-user-database")
                        .GetCluster()),
        notification_client_(
            component_context.FindComponent<UserEventsComponent>()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    const auto& name = request.GetArg("name");

    if (!name.empty()) {
      auto result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "INSERT INTO hello_schema.users(name, count) VALUES($1, 1) "
          "ON CONFLICT (name) "
          "DO UPDATE SET count = users.count + 1 "
          "RETURNING users.count",
          name);

      if (result.AsSingleRow<int>() > 1) {
        // user_type = UserType::kKnown;
      }
    }

    // TODO:
    return {};
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
  UserEventsComponent& notification_client_;
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
                            "postgres-user-database")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    return {};
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
                            "postgres-user-database")
                        .GetCluster()),
        notification_client_(context.FindComponent<UserEventsComponent>()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http ::HttpRequest& request,
      const userver::formats::json::Value& json,
      userver::server::request::RequestContext&) const override {
    return {};
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
  UserEventsComponent& notification_client_;
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
                            "postgres-user-database")
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
      .Append<UserEventsComponent>()
      .Append<userver::components::Postgres>("postgres-user-database")
      .Append<userver::clients::dns::Component>();
}

}  // namespace messenger::user_management
