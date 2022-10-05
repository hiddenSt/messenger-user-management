#include "user_events_component.hpp"

namespace messenger::user_management {

UserEventsComponent::UserEventsComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::RabbitMQ(config, context), rabbit_client_{GetClient()} {}

UserEventsComponent::~UserEventsComponent() {}

}  // namespace messenger::user_management