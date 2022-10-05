#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

namespace messenger::user_management {

void AppendUserManagement(userver::components::ComponentList& component_list);

}  // namespace messenger::user_management
