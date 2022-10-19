#include "user_events_component.hpp"

#include <userver/engine/deadline.hpp>

namespace messenger::user_management {

UserEventsComponent::UserEventsComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::RabbitMQ{config, context}, rabbit_client_{GetClient()}
    {
        // const auto setup_deadline =
        // userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
        //auto admin_channel = rabbit_client_->GetAdminChannel(setup_deadline);
        // admin_channel.DeclareExchange(
        // exchange_, userver::urabbitmq::Exchange::Type::kFanOut, setup_deadline);
        // admin_channel.DeclareQueue(queue_, setup_deadline);
        // admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
    }

void UserEventsComponent::NotifyUserIsDeleted(std::uint64_t id)
{
    std::string message = std::to_string(id);
    rabbit_client_->PublishReliable(exchange_, "user.deleted", message, userver::urabbitmq::MessageType::kTransient,
        userver::engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
}

void UserEventsComponent::NotifyNewUserIsCreated(std::uint64_t id)
{
    std::string message = std::to_string(id);
    rabbit_client_->PublishReliable(exchange_, "user.created", message, userver::urabbitmq::MessageType::kTransient,
        userver::engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
}

UserEventsComponent::~UserEventsComponent() {}

}  // namespace messenger::user_management