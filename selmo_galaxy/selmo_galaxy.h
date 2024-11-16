#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/time/real_time_clock.h"


#include <cinttypes>

namespace esphome {
namespace selmo_galaxy { 

// Temperature
const uint8_t TEMP_MIN = 10;  // Celsius
const uint8_t TEMP_MAX = 30;  // Celsius

class SelmoIrClimate : public climate_ir::ClimateIR {
 public:
  SelmoIrClimate()
      : climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 1.0f, false, false,
                              {},
                              {},
                              {climate::CLIMATE_PRESET_HOME, // presets - see climate_mode.h
                               climate::CLIMATE_PRESET_ECO,
                               climate::CLIMATE_PRESET_BOOST,
                               climate::CLIMATE_PRESET_COMFORT,
                               climate::CLIMATE_PRESET_AWAY,
                               climate::CLIMATE_PRESET_NONE,
                               climate::CLIMATE_PRESET_SLEEP}) {}

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    send_swing_cmd_ = call.get_swing_mode().has_value();
    // swing resets after unit powered off
    if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_OFF)
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    climate_ir::ClimateIR::control(call);
  }
  void set_header_high(uint32_t header_high) { this->header_high_ = header_high; }
  void set_header_low(uint32_t header_low) { this->header_low_ = header_low; }
  void set_bit_high(uint32_t bit_high) { this->bit_high_ = bit_high; }
  void set_bit_one_low(uint32_t bit_one_low) { this->bit_one_low_ = bit_one_low; }
  void set_bit_zero_low(uint32_t bit_zero_low) { this->bit_zero_low_ = bit_zero_low; }
  void set_clock(time::RealTimeClock *clock);

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;

  bool send_swing_cmd_{false};

  uint32_t frame1_(ESPTime time);
  uint32_t frame2_(uint16_t mode, uint8_t temperature);
  uint32_t schedule_frame_(uint8_t day_code, uint8_t schedule[]);
  uint8_t crc8_(uint32_t value);
  void transmit_(uint64_t value);
  
  uint32_t header_high_;
  uint32_t header_low_;
  uint32_t bit_high_;
  uint32_t bit_one_low_;
  uint32_t bit_zero_low_;

  esphome::time::RealTimeClock *clock;

  climate::ClimateMode mode_before_{climate::CLIMATE_MODE_OFF};
};

}  // namespace selmo_galaxy
}  // namespace esphome
