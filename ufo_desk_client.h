#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>

namespace esphome::ufo_desk {

enum class UfoDeskEventType : int {
  curHeightChanged = 0,
  storedHeightChanged,
  errorChanged,
  positionStatusChanged,
  btnStateChanged,
};

enum class Button : int {
  none,
  mem,
  up,
  down,
  preset1,
  preset2,
};

enum class PositionStatus : int {
  unknown = 0,
  maxOut,
  bottomOut,
  betweenMinMax,
  movingSlow,
  movingFast,
  reset,
};

class UfoDeskClient;
struct UfoDeskEvent {
  UfoDeskEvent(UfoDeskEventType type_, UfoDeskClient &desk_) : type(type_), desk(desk_) {}
  UfoDeskEventType type;
  UfoDeskClient &desk;
};

class UfoDeskClient {
 public:
  static constexpr unsigned int kResponseSize = 14;

  using UfoDeskEventHandler = std::function<void(UfoDeskEvent)>;

  void push_button(Button b);
  void release_button();
  Button pushed_button() const;

  int cur_height_mm() const;
  int stored_height_mm() const;
  int error_code() const;
  PositionStatus position_status() const;
  const std::string &position_status_str() const;

  const uint8_t *request_buf() const;
  unsigned int request_buf_size() const;

  bool parse_response(const std::array<uint8_t, kResponseSize> &resp_buf);

  void set_event_handler(UfoDeskEventHandler &&handler);

  static uint16_t crc16_modbus(const unsigned char *buf, unsigned int len);
  static const std::string &pos_status_to_str(PositionStatus s);

 private:
  void update_request_checksum();
  void publish_event(UfoDeskEventType event_type);

  int cur_height_mm_ = 0;
  int stored_height_mm_ = 0;
  int error_code_ = -1;
  PositionStatus position_status_ = PositionStatus::unknown;
  Button pushed_button_ = Button::none;

  UfoDeskEventHandler event_handler_;

  std::array<uint8_t, 8> request_buf_ = {0xea, 0xf5, 0x22, 0x00, 0x00, 0x22, 0x8a, 0x45};
};

}  // namespace esphome::ufo_desk
