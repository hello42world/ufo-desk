#include "ufo_desk_client.h"
#include "esphome/core/log.h"

namespace esphome::ufo_desk {

static const char *TAG = "ufo_desk.client";

void UfoDeskClient::push_button(Button b) {
  ESP_LOGD(TAG, "Button pushed:%d", b);
  switch (b) {
    case Button::none:
      request_buf_[2] = 0x22;
      break;
    case Button::mem:
      request_buf_[2] = 0x40;
      break;
    case Button::up:
      request_buf_[2] = 0x11;
      break;
    case Button::down:
      request_buf_[2] = 0x33;
      break;
    case Button::preset1:
      request_buf_[2] = 0x41;
      break;
    case Button::preset2:
      request_buf_[2] = 0x42;
      break;
  }
  request_buf_[5] = request_buf_[2];
  this->update_request_checksum();
  if (b != pushed_button_) {
    pushed_button_ = b;
    this->publish_event(UfoDeskEventType::btnStateChanged);
  }
}

void UfoDeskClient::release_button() { this->push_button(Button::none); }

Button UfoDeskClient::pushed_button() const { return pushed_button_; }

int UfoDeskClient::cur_height_mm() const { return cur_height_mm_; }

int UfoDeskClient::stored_height_mm() const { return stored_height_mm_; }

int UfoDeskClient::error_code() const { return error_code_; }

PositionStatus UfoDeskClient::position_status() const { return position_status_; }

const std::string &UfoDeskClient::position_status_str() const { return pos_status_to_str(position_status_); }

const uint8_t *UfoDeskClient::request_buf() const { return &request_buf_[0]; }

unsigned int UfoDeskClient::request_buf_size() const { return request_buf_.size(); }

bool UfoDeskClient::parse_response(const std::array<uint8_t, kResponseSize> &resp_buf) {
  // Check header.
  if (resp_buf[0] != 0xe4 || resp_buf[1] != 0xf7) {
    // Bad header.
    return false;
  }

  // Check crc
  if (crc16_modbus(&resp_buf[2], 10) != *reinterpret_cast<const uint16_t *>(&resp_buf[12])) {
    // CRC error.
    return false;
  }

  // Current height.
  constexpr int h_min = 720;
  constexpr int h_max = 1200;
  auto cur_height_mm = h_min + (resp_buf[8] + (resp_buf[7] << 8) - 0x2d) * (h_max - h_min) / 0x1263;
  if (cur_height_mm != cur_height_mm_) {
    cur_height_mm_ = cur_height_mm;
    publish_event(UfoDeskEventType::curHeightChanged);
  }

  // Stored height.
  auto stored_height_mm = h_min + (resp_buf[6] + (resp_buf[5] << 8) - 0x2d) * (h_max - h_min) / 0x1263;
  if (stored_height_mm != stored_height_mm_) {
    stored_height_mm_ = stored_height_mm;
    publish_event(UfoDeskEventType::storedHeightChanged);
  }

  // Position status.
  PositionStatus position_status = PositionStatus::unknown;
  switch (resp_buf[11]) {
    case 0x00:
      position_status = PositionStatus::bottomOut;
      break;
    case 0x01:
      position_status = PositionStatus::betweenMinMax;
      break;
    case 0x02:
      position_status = PositionStatus::maxOut;
      break;
    case 0x10:
      position_status = PositionStatus::movingSlow;
      break;
    case 0x20:
      position_status = PositionStatus::movingFast;
      break;
    case 0x04:
      position_status = PositionStatus::reset;
      break;
  }
  if (position_status == PositionStatus::unknown) {
    ESP_LOGW(TAG, "Unknown position status: %02x", resp_buf[11]);
  }
  if (position_status != position_status_) {
    position_status_ = position_status;
    publish_event(UfoDeskEventType::positionStatusChanged);
  }

  // Error.
  auto err = resp_buf[4];
  if (err != error_code_) {
    error_code_ = err;
    publish_event(UfoDeskEventType::errorChanged);
  }

  return true;
}

void UfoDeskClient::set_event_handler(esphome::ufo_desk::UfoDeskClient::UfoDeskEventHandler &&handler) {
  event_handler_ = std::move(handler);
}

uint16_t UfoDeskClient::crc16_modbus(const uint8_t *buf, unsigned int len) {
  static const uint16_t table[2] = {0x0000, 0xA001};
  uint16_t crc = 0xFFFF;
  unsigned int i = 0;
  char bit = 0;
  unsigned int xxor = 0;

  for (i = 0; i < len; i++) {
    crc ^= buf[i];
    for (bit = 0; bit < 8; bit++) {
      xxor = crc & 0x01;
      crc >>= 1;
      crc ^= table[xxor];
    }
  }
  return crc;
}

const std::string &UfoDeskClient::pos_status_to_str(PositionStatus s) {
  static const std::string vars[] = {"Unknown",     "Maxed out",   "Bottomed out", "Between min max",
                                     "Moving slow", "Moving fast", "Reset"};
  return vars[static_cast<int>(s)];
}

void UfoDeskClient::update_request_checksum() {
  uint16_t check_sum = crc16_modbus(&request_buf_[2], 4);
  request_buf_[6] = check_sum & 0x00ff;
  request_buf_[7] = check_sum >> 8;
}

void UfoDeskClient::publish_event(UfoDeskEventType event_type) {
  if (event_handler_) {
    event_handler_(UfoDeskEvent(event_type, *this));
  }
}

}  // namespace esphome::ufo_desk
