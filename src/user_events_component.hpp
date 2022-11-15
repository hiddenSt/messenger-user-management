#pragma once

#include <cstdint>
#include <memory>

#include <userver/rabbitmq.hpp>

namespace messenger::user_management {

namespace components = userver::components;
namespace rabbitmq = userver::urabbitmq;

class UserEventsComponent final : public components::RabbitMQ {
 public:
  static constexpr std::string_view kName = "user-events";

  UserEventsComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

  ~UserEventsComponent() override;

  void NotifyUserIsDeleted(std::int32_t id);
  void NotifyNewUserIsCreated(std::int32_t id);

 private:
  const rabbitmq::Exchange exchange_{"user-events"};
  const rabbitmq::Queue removed_event_queue_{"user-removed-queue"};
  const rabbitmq::Queue added_event_queue_{"user-created-queue"};

  std::shared_ptr<rabbitmq::Client> rabbit_client_;
};

}  // namespace messenger::user_management
