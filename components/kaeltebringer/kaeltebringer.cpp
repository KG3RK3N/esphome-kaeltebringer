#include "kaeltebringer.h"

namespace esphome {
namespace kaeltebringer {

  static const char *const TAG = "kaeltebringer";

  void KaeltebringerClimate::setup() {
    // This will be called by App.setup()
  }

  void KaeltebringerClimate::set_beep_enabled(bool enabled) { this->beep_enabled_ = enabled; }

  void KaeltebringerClimate::set_current_temperature(float current_temperature) {
    if (this->current_temperature == current_temperature) return;
    this->current_temperature = current_temperature;
    this->is_changed = true;
  }

  void KaeltebringerClimate::set_custom_fan_mode(const std::string &fan_mode) {
    if (this->custom_fan_mode == fan_mode) return;
    this->custom_fan_mode = fan_mode;
    this->is_changed = true;
  }

  void KaeltebringerClimate::set_mode(esphome::climate::ClimateMode mode) {
    if (this->mode == mode) return;
    this->mode = mode;
    this->is_changed = true;
  }

  void KaeltebringerClimate::set_swing_mode(esphome::climate::ClimateSwingMode swing_mode) {
    if (this->swing_mode == swing_mode) return;
    this->swing_mode = swing_mode;
    this->is_changed = true;
  }

  void KaeltebringerClimate::set_target_temperature(float target_temperature) {
    if (this->target_temperature == target_temperature) return;
    this->target_temperature = target_temperature;
    this->is_changed = true;
  }

  void KaeltebringerClimate::build_set_cmd(get_cmd_resp_t * get_cmd_resp) {
    memcpy(m_set_cmd.raw, set_cmd_base, sizeof(m_set_cmd.raw));

    m_set_cmd.data.power = get_cmd_resp->data.power;
    m_set_cmd.data.off_timer_en = 0;
    m_set_cmd.data.on_timer_en = 0;
    m_set_cmd.data.beep = int(this->beep_enabled_);
    m_set_cmd.data.disp = 1;
    m_set_cmd.data.eco = 0;

    switch (get_cmd_resp->data.mode) {
      case 0x01:
        m_set_cmd.data.mode = 0x03;
        break;
      case 0x03:
        m_set_cmd.data.mode = 0x02;
        break;
      case 0x02:
        m_set_cmd.data.mode = 0x07;
        break;
      case 0x04:
        m_set_cmd.data.mode = 0x01;
        break;
      case 0x05:
        m_set_cmd.data.mode = 0x08;
        break;
    }

    m_set_cmd.data.turbo = get_cmd_resp->data.turbo;
    m_set_cmd.data.mute = get_cmd_resp->data.mute;
    m_set_cmd.data.temp = 15 - get_cmd_resp->data.temp;

    switch (get_cmd_resp->data.fan) {
      case 0x00:
        m_set_cmd.data.fan = 0x00;
        break;
      case 0x01:
        m_set_cmd.data.fan = 0x02;
        break;
      case 0x04:
        m_set_cmd.data.fan = 0x06;
        break;
      case 0x02:
        m_set_cmd.data.fan = 0x03;
        break;
      case 0x05:
        m_set_cmd.data.fan = 0x07;
        break;
      case 0x03:
        m_set_cmd.data.fan = 0x05;
        break;
    }

    m_set_cmd.data.vswing = get_cmd_resp->data.vswing ? 0x07 : 0x00;
    m_set_cmd.data.hswing = get_cmd_resp->data.hswing;

    m_set_cmd.data.half_degree = 0;

    for (int i = 0; i < sizeof(m_set_cmd.raw) - 1; i++) m_set_cmd.raw[sizeof(m_set_cmd.raw) - 1] ^= m_set_cmd.raw[i];
  }
  
  int KaeltebringerClimate::read_data_line(int readch, uint8_t *buffer, int len)
  {
    static int pos = 0;
    static bool wait_len = false;
    static int skipch = 0;

    //ESP_LOGD("custom", "%02X", readch);

    if (readch >= 0) {
      if (readch == 0xBB && skipch == 0 && !wait_len) {
        pos = 0;
        skipch = 3; // wait char with len
        wait_len = true;
        if (pos < len-1) buffer[pos++] = readch;
      } else if (skipch == 0 && wait_len) {
        if (pos < len-1) buffer[pos++] = readch;
        skipch = readch + 1; // +1 control sum
        ESP_LOGD(TAG, "len: %d", readch);
        wait_len = false;
      } else if (skipch > 0) {
        if (pos < len-1) buffer[pos++] = readch;
        if (--skipch == 0 && !wait_len) return pos;
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }

  void KaeltebringerClimate::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value()) {
      // User requested mode change
      climate::ClimateMode climate_mode = *call.get_mode();
      // Send mode to hardware

      get_cmd_resp_t get_cmd_resp = {0};
      memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

      if (climate_mode == climate::CLIMATE_MODE_OFF) {
        get_cmd_resp.data.power = 0x00;
      } else {
        get_cmd_resp.data.power = 0x01;
        switch (climate_mode) {
          case climate::CLIMATE_MODE_COOL:
            get_cmd_resp.data.mode = 0x01;
            break;
          case climate::CLIMATE_MODE_DRY:
            get_cmd_resp.data.mode = 0x03;
            break;
          case climate::CLIMATE_MODE_FAN_ONLY:
            get_cmd_resp.data.mode = 0x02;
            break;
          case climate::CLIMATE_MODE_HEAT:
            get_cmd_resp.data.mode = 0x04;
            break;
          case climate::CLIMATE_MODE_AUTO:
            get_cmd_resp.data.mode = 0x05;
            break;
        }
      }

      build_set_cmd(&get_cmd_resp);
      ready_to_send_set_cmd_flag = true;

      // Publish updated state
    //  this->mode = mode;
     // this->publish_state();
    }
    if (call.get_target_temperature().has_value()) {
      // User requested target temperature change
      float temp = *call.get_target_temperature();
      
      get_cmd_resp_t get_cmd_resp = {0};
      memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

      get_cmd_resp.data.temp = uint8_t(temp) - 16;

      build_set_cmd(&get_cmd_resp);
      ready_to_send_set_cmd_flag = true;
    }
    if (call.get_swing_mode().has_value()) {
      // User requested target temperature change
      esphome::climate::ClimateSwingMode swing_mode = *call.get_swing_mode();

      get_cmd_resp_t get_cmd_resp = {0};
      memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

      switch(swing_mode) {
        case climate::CLIMATE_SWING_OFF:
          get_cmd_resp.data.hswing = 0;
          get_cmd_resp.data.vswing = 0;
          break;
        case climate::CLIMATE_SWING_BOTH:
          get_cmd_resp.data.hswing = 1;
          get_cmd_resp.data.vswing = 1;
          break;
        case climate::CLIMATE_SWING_VERTICAL:
          get_cmd_resp.data.hswing = 0;
          get_cmd_resp.data.vswing = 1;
          break;
        case climate::CLIMATE_SWING_HORIZONTAL:
          get_cmd_resp.data.hswing = 1;
          get_cmd_resp.data.vswing = 0;
          break;
      }

      build_set_cmd(&get_cmd_resp);
      ready_to_send_set_cmd_flag = true;
    }
    if (call.get_custom_fan_mode().has_value()) {
      // User requested target temperature change
      std::string fan_mode = *call.get_custom_fan_mode();

      get_cmd_resp_t get_cmd_resp = {0};
      memcpy(get_cmd_resp.raw, m_get_cmd_resp.raw, sizeof(get_cmd_resp.raw));

      get_cmd_resp.data.turbo = 0x00;
      get_cmd_resp.data.mute = 0x00;
      if (fan_mode == std::string("Turbo")) { 
        get_cmd_resp.data.fan = 0x03;
        get_cmd_resp.data.turbo = 0x01;
      } else if (fan_mode == std::string("Mute")) {
        get_cmd_resp.data.fan = 0x01;
        get_cmd_resp.data.mute = 0x01;
      } else if (fan_mode == std::string("Automatic")) get_cmd_resp.data.fan = 0x00;
      else if (fan_mode == std::string("1")) get_cmd_resp.data.fan = 0x01;
      else if (fan_mode == std::string("2")) get_cmd_resp.data.fan = 0x04;
      else if (fan_mode == std::string("3")) get_cmd_resp.data.fan = 0x02;
      else if (fan_mode == std::string("4")) get_cmd_resp.data.fan = 0x05;
      else if (fan_mode == std::string("5")) get_cmd_resp.data.fan = 0x03;

      build_set_cmd(&get_cmd_resp);
      ready_to_send_set_cmd_flag = true;
    }
    update();
  }

  bool KaeltebringerClimate::is_valid_xor(uint8_t *buffer, int len)
  {
    uint8_t xor_byte = 0;
    for (int i = 0; i < len - 1; i++) xor_byte ^= buffer[i];
    if (xor_byte == buffer[len - 1]) return true;
    else {
      ESP_LOGW(TAG, "No valid xor crc %02X (calculated %02X)", buffer[len], xor_byte);
      return false;
    }
    
  }

  void KaeltebringerClimate::print_hex_str(uint8_t *buffer, int len)
  {
    char str[250] = {0};
    char *pstr = str;
    if (len * 2 > sizeof(str)) ESP_LOGE(TAG, "too long byte data");

    for (int i = 0; i < len; i++) {
      pstr += sprintf(pstr, "%02X ", buffer[i]);
    }

    ESP_LOGD(TAG, "%s", str);
  }

  void KaeltebringerClimate::update() {
    // This will be called every "update_interval" milliseconds.
    uint8_t req_cmd[] = {0xBB, 0x00, 0x01, 0x04, 0x02, 0x01, 0x00, 0xBD};

    if (ready_to_send_set_cmd_flag) {
        ready_to_send_set_cmd_flag = false;
        write_array(m_set_cmd.raw, sizeof(m_set_cmd.raw));
    }
    else write_array(req_cmd, sizeof(req_cmd));
  }

  void KaeltebringerClimate::loop() {
    const int max_line_length = 100;
    static uint8_t buffer[max_line_length];
    
    while (available()) {
      int len = read_data_line(read(), buffer, max_line_length);
      if(len == sizeof(m_get_cmd_resp) && buffer[3] == 0x04) {
        memcpy(m_get_cmd_resp.raw, buffer, len);
        print_hex_str(buffer, len);
        if (is_valid_xor(buffer, len)) {
          float curr_temp = (((buffer[17] << 8) | buffer[18]) / 374 - 32) / 1.8;

          if (m_get_cmd_resp.data.power == 0x00) this->set_mode(climate::CLIMATE_MODE_OFF);
          else if (m_get_cmd_resp.data.mode == 0x01) this->set_mode(climate::CLIMATE_MODE_COOL);
          else if (m_get_cmd_resp.data.mode == 0x03) this->set_mode(climate::CLIMATE_MODE_DRY);
          else if (m_get_cmd_resp.data.mode == 0x02) this->set_mode(climate::CLIMATE_MODE_FAN_ONLY);
          else if (m_get_cmd_resp.data.mode == 0x04) this->set_mode(climate::CLIMATE_MODE_HEAT);
          else if (m_get_cmd_resp.data.mode == 0x05) this->set_mode(climate::CLIMATE_MODE_AUTO);


          if (m_get_cmd_resp.data.turbo) this->set_custom_fan_mode(std::string("Turbo"));
          else if (m_get_cmd_resp.data.mute) this->set_custom_fan_mode(std::string("Mute"));
          else if (m_get_cmd_resp.data.fan == 0x00) this->set_custom_fan_mode(std::string("Automatic"));
          else if (m_get_cmd_resp.data.fan == 0x01) this->set_custom_fan_mode(std::string("1"));
          else if (m_get_cmd_resp.data.fan == 0x04) this->set_custom_fan_mode(std::string("2"));
          else if (m_get_cmd_resp.data.fan == 0x02) this->set_custom_fan_mode(std::string("3"));
          else if (m_get_cmd_resp.data.fan == 0x05) this->set_custom_fan_mode(std::string("4"));
          else if (m_get_cmd_resp.data.fan == 0x03) this->set_custom_fan_mode(std::string("5"));


          if (m_get_cmd_resp.data.hswing && m_get_cmd_resp.data.vswing) this->set_swing_mode(climate::CLIMATE_SWING_BOTH);
          else if (!m_get_cmd_resp.data.hswing && !m_get_cmd_resp.data.vswing) this->set_swing_mode(climate::CLIMATE_SWING_OFF);
          else if (m_get_cmd_resp.data.vswing) this->set_swing_mode(climate::CLIMATE_SWING_VERTICAL);
          else if (m_get_cmd_resp.data.hswing) this->set_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);


          ESP_LOGD(TAG, "fan %02X", m_get_cmd_resp.data.fan);
          ESP_LOGD(TAG, "mode %02X", m_get_cmd_resp.data.mode);
          this->set_target_temperature(float(m_get_cmd_resp.data.temp + 16));
          this->set_current_temperature(curr_temp);
          if (this->is_changed)
             this->publish_state();
        }
      }
    }
  }

}
}