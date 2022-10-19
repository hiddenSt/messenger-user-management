#include "user_events_component.hpp"

#include <string>

#include <userver/engine/deadline.hpp>

namespace messenger::user_management {

namespace engine = userver::engine;

const std::string kUserCreatedRoutingKey = "user.created";
const std::string kUserRemovedRoutingKey = "user.removed";

UserEventsComponent::UserEventsComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::RabbitMQ{config, context}, rabbit_client_{GetClient()} {
  const auto setup_deadline =
      engine::Deadline::FromDuration(std::chrono::seconds{2});
  auto admin_channel = rabbit_client_->GetAdminChannel(setup_deadline);
  admin_channel.DeclareExchange(exchange_, rabbitmq::Exchange::Type::kDirect,
                                setup_deadline);

  admin_channel.DeclareQueue(added_event_queue_, setup_deadline);
  admin_channel.BindQueue(exchange_, added_event_queue_, kUserCreatedRoutingKey,
                          setup_deadline);

  admin_channel.DeclareQueue(removed_event_queue_, setup_deadline);
  admin_channel.BindQueue(exchange_, removed_event_queue_,
                          kUserRemovedRoutingKey, setup_deadline);
}

void UserEventsComponent::NotifyUserIsDeleted(std::int32_t id) {
  std::string message = std::to_string(id);
  rabbit_client_->PublishReliable(
      exchange_, kUserRemovedRoutingKey, message,
      rabbitmq::MessageType::kTransient,
      engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
}

void UserEventsComponent::NotifyNewUserIsCreated(std::int32_t id) {
  std::string message = std::to_string(id);
  rabbit_client_->PublishReliable(
      exchange_, kUserCreatedRoutingKey, message,
      rabbitmq::MessageType::kTransient,
      engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
}

UserEventsComponent::~UserEventsComponent() {}

}  // namespace messenger::user_management