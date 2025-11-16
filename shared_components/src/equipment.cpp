#include "equipment.h"

void Equipment::update_status(const std::string &new_status) {
  status_ = new_status;
}
void Equipment::update_equipment_power_state(
    const std::string &new_power_state) {
  power_state_ = new_power_state;
}

void Equipment::update_heartbeat() { last_heartbeat_ = time(nullptr); }

bool Equipment::is_online() const {
  return (time(nullptr) - last_heartbeat_ < 60);
}