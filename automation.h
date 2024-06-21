#pragma once

#include "esphome/core/automation.h"
#include "ufo_desk.h"

namespace esphome::ufo_desk {

class UfoDeskTrigger : public Trigger<UfoDeskEvent> {
 public:
  explicit UfoDeskTrigger(UfoDesk *parent) {
    parent->add_event_callback([this](UfoDeskEvent e) { this->trigger(e); });
  }
};

}  // namespace esphome::ufo_desk
