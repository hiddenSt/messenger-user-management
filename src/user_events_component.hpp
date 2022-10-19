#pragma once

#include <cstdint>
#include <memory>

#include <userver/rabbitmq.hpp>

namespace messenger::user_management {

namespace components = userver::components;

class UserEventsComponent final : public userver::components::RabbitMQ {
 public:
  static constexpr std::string_view kName = "user-events";

  UserEventsComponent(const components::ComponentConfig& config,
                      const components::ComponentContext& context);

  ~UserEventsComponent() override;

  void NotifyUserIsDeleted(std::uint64_t id);
  void NotifyNewUserIsCreated(std::uint64_t id);

 private:
  const userver::urabbitmq::Exchange exchange_;
  const userver::urabbitmq::Queue queue_;

  std::shared_ptr<userver::urabbitmq::Client> rabbit_client_;
};

}  // namespace messenger::user_management
