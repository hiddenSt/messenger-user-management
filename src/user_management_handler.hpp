#pragma once

#include <userver/components/component_list.hpp>

namespace messenger::user_management {

namespace components = userver::components;

void AppendUserManagement(components::ComponentList& component_list);

}  // namespace messenger::user_management
