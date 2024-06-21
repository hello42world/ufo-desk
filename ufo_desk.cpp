#include "esphome/core/log.h"
#include "ufo_desk.h"

namespace esphome::ufo_desk {

static const char *TAG = "ufo_desk.component";

UfoDesk::UfoDesk() {}

void UfoDesk::setup() {
  this->set_interval("uart_ping_pong", 200, [this]() {
    // ESP_LOGD(TAG, "Sending request to control box");
    this->write_array(this->desk_client_.request_buf(), this->desk_client_.request_buf_size());
    this->response_pending_ = true;
    this->resp_cur_size_ = 0;
    this->request_sent_at_ = millis();
  });

  desk_client_.set_event_handler([this](UfoDeskEvent e) { this->event_callbacks_.call(e); });
}

void UfoDesk::loop() {
  while (this->available()) {
    uint8_t b = 0;
    if (!this->read_byte(&b) || !this->response_pending_) {
      return;
    }

    if (millis() - request_sent_at_ >= kRESPONSE_TIMEOUT_MS) {
      response_pending_ = false;
      return;
    }

    response_buffer_[resp_cur_size_++] = b;
    if (resp_cur_size_ == response_buffer_.size()) {
      //      ESP_LOGD(TAG, "Response from control box received");
      response_pending_ = false;
      if (desk_client_.parse_response(response_buffer_)) {
        uint16_t crc = *reinterpret_cast<uint16_t *>(&response_buffer_[12]);
        if (crc != dbg_last_crc_) {
          ESP_LOGD(TAG, "Resp: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", response_buffer_[2],
                   response_buffer_[3], response_buffer_[4], response_buffer_[5], response_buffer_[6],
                   response_buffer_[7], response_buffer_[8], response_buffer_[9], response_buffer_[10],
                   response_buffer_[11]);
        }
        dbg_last_crc_ = crc;
      } else {
        ESP_LOGW(TAG, "Failed to parse response buffer");
      }
    }
  }
}

void UfoDesk::update() {}

void UfoDesk::dump_config() { ESP_LOGCONFIG(TAG, "UFO Desk"); }

void UfoDesk::add_event_callback(std::function<void(UfoDeskEvent)> &&callback) {
  event_callbacks_.add(std::move(callback));
}

}  // namespace esphome::ufo_desk
