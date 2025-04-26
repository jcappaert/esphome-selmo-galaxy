#include "selmo_galaxy.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace selmo_galaxy {

static const char *const TAG = "climate.selmo_galaxy";

const uint8_t MON_CODE = 0x08;
const uint8_t TUE_CODE = 0x09;
const uint8_t WED_CODE = 0x0A;
const uint8_t THU_CODE = 0x0B;
const uint8_t FRI_CODE = 0x0C;
const uint8_t SAT_CODE = 0x0D;
const uint8_t SUN_CODE = 0x0E;

const uint8_t FRAME1_HOUR_FOR_COMPLETE_DAY = 32;

const uint8_t FRAME2_MODE_OFF = 0x04;
const uint8_t FRAME2_MODE_FIL_PILOTE = 0x10;
const uint8_t FRAME2_MODE_COMFORT = 0x01;
const uint8_t FRAME2_MODE_NIGHT = 0x02;
const uint8_t FRAME2_MODE_ANTI_ICE = 0x08;
const uint8_t FRAME2_MODE_ECO = 0x06;
const uint8_t FRAME2_TEMP_SHIFT_BITS = 8;
const uint8_t FRAME2_TEMP_MULTIPLIER = 20;
const uint8_t FRAME2_MODE_SHIFT_BITS = 18;

const uint32_t FRAME10 = 0x02400200;

const uint32_t FRAME1_EXAMPLE = 0x00151726;
const uint32_t FRAME2_EXAMPLE = 0x010658aa;
const uint32_t FRAME3_EXAMPLE = MON_CODE << 24;
const uint32_t FRAME4_EXAMPLE = TUE_CODE << 24;
const uint32_t FRAME5_EXAMPLE = WED_CODE << 24;
const uint32_t FRAME6_EXAMPLE = THU_CODE << 24;
const uint32_t FRAME7_EXAMPLE = FRI_CODE << 24; 
const uint32_t FRAME8_EXAMPLE = SAT_CODE << 24; 
const uint32_t FRAME9_EXAMPLE = SUN_CODE << 24;

const uint8_t DEFAULT_SCHEDULE[] = {0, 0, 0, 0, 0, 0,
                                    1, 1, 1, 0, 0, 0,
                                    0, 0, 0, 0, 1, 1,
                                    1, 1, 1, 1, 1, 0};


const uint16_t BITS = 40;
const uint8_t FRAME_DELAY_MS = 60;

void SelmoIrClimate::transmit_state() {

  // ESP_LOGD(TAG, "selmo_galaxy mode_before_ code: 0x%02X", modeBefore_);

  /*
  bool climate_is_off = (mode_before_ == climate::CLIMATE_MODE_OFF);
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      remote_state |= climate_is_off ? COMMAND_ON_COOL : COMMAND_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      remote_state |= climate_is_off ? COMMAND_ON_DRY : COMMAND_DRY;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      remote_state |= climate_is_off ? COMMAND_ON_FAN_ONLY : COMMAND_FAN_ONLY;
      break;
    case climate::CLIMATE_MODE_HEAT_COOL:
      remote_state |= climate_is_off ? COMMAND_ON_AI : COMMAND_AI;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state |= climate_is_off ? COMMAND_ON_HEAT : COMMAND_HEAT;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state |= COMMAND_OFF;
      break;
  }
  */


 /* 
   // register for humidity values and get initial state
  if (this->humidity_sensor_ != nullptr) {
    this->humidity_sensor_->add_on_state_callback([this](float state) {
      this->current_humidity = state;
      this->publish_state();
    });
    this->current_humidity = this->humidity_sensor_->state;
  }*/


  mode_before_ = this->mode;

  ESP_LOGD(TAG, "selmo_galaxy mode code: 0x%02X", this->mode);


  time_t epoch = (*clock).timestamp_now();
  ESP_LOGD(TAG, "%jd epoch seconds\n", (intmax_t)epoch);
  ESPTime time = (*clock).now();
  ESP_LOGD(TAG, "%i %i %i %i\n", time.day_of_week, time.hour, time.minute, time.second);



  uint32_t words[] = {frame1_((*clock).now()),
                      FRAME2_EXAMPLE,
                      schedule_frame_(MON_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(TUE_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(WED_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(THU_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(FRI_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(SAT_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      schedule_frame_(SUN_CODE, (uint8_t *)DEFAULT_SCHEDULE),
                      FRAME10};

  for (uint8_t i=0; i < sizeof(words)/sizeof(words[0]); i++) {
    if (i != 0) {
      esphome::delay_microseconds_safe(FRAME_DELAY_MS*1000);
    }

    uint64_t frame = ((uint64_t)words[i] << 8) | crc8_(words[i]);
    transmit_(frame); // transmits single frame
  }                    

  this->publish_state();
}

bool SelmoIrClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t nbits = 0;
  uint32_t remote_state = 0;

  if (!data.expect_item(this->header_high_, this->header_low_))
    return false;

  for (nbits = 0; nbits < 32; nbits++) {
    if (data.expect_item(this->bit_high_, this->bit_one_low_)) {
      remote_state = (remote_state << 1) | 1;
    } else if (data.expect_item(this->bit_high_, this->bit_zero_low_)) {
      remote_state = (remote_state << 1) | 0;
    } else if (nbits == BITS) {
      break;
    } else {
      return false;
    }
  }

  ESP_LOGD(TAG, "Decoded 0x%02" PRIX32, remote_state);

  /*
  if ((remote_state & 0xFF00000) != 0x8800000)
    return false;

  // Get command
  if ((remote_state & COMMAND_MASK) == COMMAND_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else if ((remote_state & COMMAND_MASK) == COMMAND_SWING) {
    this->swing_mode =
        this->swing_mode == climate::CLIMATE_SWING_OFF ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;
  } else {
    switch (remote_state & COMMAND_MASK) {
      case COMMAND_DRY:
      case COMMAND_ON_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case COMMAND_FAN_ONLY:
      case COMMAND_ON_FAN_ONLY:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case COMMAND_AI:
      case COMMAND_ON_AI:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case COMMAND_HEAT:
      case COMMAND_ON_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case COMMAND_COOL:
      case COMMAND_ON_COOL:
      default:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
    }

    // Get fan speed
    if (this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
    } else if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_DRY ||
               this->mode == climate::CLIMATE_MODE_FAN_ONLY || this->mode == climate::CLIMATE_MODE_HEAT) {
      if ((remote_state & FAN_MASK) == FAN_AUTO) {
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
      } else if ((remote_state & FAN_MASK) == FAN_MIN) {
        this->fan_mode = climate::CLIMATE_FAN_LOW;
      } else if ((remote_state & FAN_MASK) == FAN_MED) {
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      } else if ((remote_state & FAN_MASK) == FAN_MAX) {
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
      }
    }

    // Get temperature
    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT) {
      this->target_temperature = ((remote_state & TEMP_MASK) >> TEMP_SHIFT) + 15;
    }
  }
  this->publish_state();
  */

  return true;
}

// Transmit single frame
void SelmoIrClimate::transmit_(uint64_t value) {
  //uint8_t crc = crc8_(value);
  //uint64_t frame = (value << 8) | crc;

  ESP_LOGD(TAG, "Sending selmo_galaxy code: 0x%010" PRIX64, value);

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(38000);
  data->reserve(4 + BITS * 2u);

  data->item(this->header_high_, this->header_low_);

  for (uint8_t bit = BITS; bit > 0; bit--) {
    if ((value >> (bit - 1)) & 1) {
      data->item(this->bit_high_, this->bit_one_low_);
    } else {
      data->item(this->bit_high_, this->bit_zero_low_);
    }
  }

  data->mark(bit_high_);
  transmit.perform();
}


uint32_t SelmoIrClimate::frame2_(uint16_t mode, uint8_t temperature) {
  uint32_t frame2 = 0x00000000;

  uint16_t temp_raw = 20 * temperature;
  frame2 |= (temp_raw & 0x3FF)  << FRAME2_TEMP_SHIFT_BITS; // Temp is in there as 10-bit
  return frame2;
}

uint32_t SelmoIrClimate::frame1_(ESPTime time) {
  uint8_t days = time.day_of_week - 1; // sunday is 1
  uint8_t hour = time.hour;
  uint8_t min = time.minute;
  uint8_t sec = time.second;

  uint8_t hours = 32*days + hour;

  uint32_t frame1 = (hours << 16) | (min << 8) | sec;
  return frame1;
}

uint32_t SelmoIrClimate::schedule_frame_(uint8_t day_code, uint8_t schedule[]) {
    uint32_t encoded = 0;
    // Reverse the bits and build the 24-bit integer
    for (int i = 0; i < 24; ++i) {
        encoded |= (schedule[23 - i] << i);
    }

  return (day_code << 24) | (encoded & 0xFFFFFF);
}

// CRC8, DALLAS-DOW implementation with polynomial 0x31
uint8_t SelmoIrClimate::crc8_(uint32_t value) {
  uint8_t polynomial = 0x31;
  uint32_t crc = 0x00; // initial_value
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t b = (value >> (8 * (3-i))) & 0xFF;
    b = esphome::reverse_bits(b);
    crc ^= b; // XOR the byte into the current CRC value
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ polynomial;
      }
      else {
        crc <<= 1;
      }
      crc &= 0xFF;  // Ensure CRC remains within 8 bits
    }
  }
  return esphome::reverse_bits((uint8_t)crc);
}

void SelmoIrClimate::set_clock(time::RealTimeClock *clock) {
  this->clock = clock;
  ESP_LOGD(TAG, "set_clock");
}

}  // namespace selmo_galaxy
}  // namespace esphome