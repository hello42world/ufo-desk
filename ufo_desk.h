#pragma once

#include <array>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#include "ufo_desk_client.h"

namespace esphome::ufo_desk {

class UfoDesk : public uart::UARTDevice, public PollingComponent {
 public:
  UfoDesk();

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  UfoDeskClient &desk() { return desk_client_; }

  void add_event_callback(std::function<void(UfoDeskEvent)> &&callback);

 protected:
 private:
  static constexpr uint32_t kRESPONSE_TIMEOUT_MS = 100;

  bool response_pending_ = false;

  uint32_t request_sent_at_ = 0;
  unsigned int resp_cur_size_ = 0;
  std::array<uint8_t, UfoDeskClient::kResponseSize> response_buffer_;

  UfoDeskClient desk_client_;

  uint16_t dbg_last_crc_ = 0;

  CallbackManager<void(UfoDeskEvent)> event_callbacks_;
};

template<typename S, size_t N> void exclusive_turn_on(S *on, const std::array<S *, N> &all) {
  for (const auto &s : all) {
    if (on == s) {
      if (!s->state) {
        s->turn_on();
      }
    } else {
      if (s->state) {
        s->turn_off();
      }
    }
  }
}

}  // namespace esphome::ufo_desk
