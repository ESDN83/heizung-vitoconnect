/*
  optolink.cpp - Connect Viessmann heating devices via Optolink to ESPhome

  Copyright (C) 2023  Philipp Danner

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vitoconnect.h"

namespace esphome {
namespace vitoconnect {

static const char *TAG = "vitoconnect";

void VitoConnect::setup() {

    // ESDN83: check_uart_settings() entfernt (deprecated, faellt mit ESPHome
    // 2027.3.0 weg). Die UART-Parameter prueft jetzt FINAL_VALIDATE_SCHEMA in
    // __init__.py schon beim Validieren der Konfiguration.

    ESP_LOGD(TAG, "Starting optolink with protocol: %s", this->protocol.c_str());
    if (this->protocol.compare("P300") == 0) {
        _optolink = new OptolinkP300(this);
    } else if (this->protocol.compare("KW") == 0) {
        _optolink = new OptolinkKW(this);
    } else if (this->protocol.compare("GWG") == 0) {
        _optolink = new OptolinkGWG(this);
    } else {
      ESP_LOGW(TAG, "Unknown protocol.");
    }

    // optimize datapoint list
    _datapoints.shrink_to_fit();

    if (_optolink) {

      // add onData and onError callbacks
      _optolink->onData(&VitoConnect::_onData);
      _optolink->onError(&VitoConnect::_onError);
      
      // set initial state
      _optolink->begin();

    } else {
      ESP_LOGW(TAG, "Not able to initialize VitoConnect");
    }
}

void VitoConnect::register_datapoint(Datapoint *datapoint) {
    ESP_LOGD(TAG, "Adding datapoint with address %x and length %d", datapoint->getAddress(), datapoint->getLength());
    this->_datapoints.push_back(datapoint);
}

void VitoConnect::loop() {
    if (_optolink) _optolink->loop();
}

void VitoConnect::update() {
  // This will be called every "update_interval" milliseconds.
  ESP_LOGD(TAG, "Schedule sensor update");
  
  for (Datapoint* dp : this->_datapoints) {
      CbArg* arg = new CbArg(this, dp);   
      if (_optolink->read(dp->getAddress(), dp->getLength(), reinterpret_cast<void*>(arg))) {
      } else {
          delete arg;
      }
  }
}

bool VitoConnect::read_raw(uint16_t address, uint8_t length, RawCallback on_data,
                           RawErrorCallback on_error) {
  if (!_optolink || length == 0 || length > MAX_DP_LENGTH) return false;
  CbArg* arg = new CbArg(this, std::move(on_data), std::move(on_error));
  if (_optolink->read(address, length, reinterpret_cast<void*>(arg))) return true;
  ESP_LOGW(TAG, "read_raw %04X: Queue voll", address);
  delete arg;
  return false;
}

bool VitoConnect::write_raw(uint16_t address, uint8_t length, const uint8_t* data,
                            RawCallback on_data, RawErrorCallback on_error) {
  if (!_optolink || length == 0 || length > MAX_DP_LENGTH) return false;
  // Optolink::write kopiert die Daten, erwartet aber einen nicht-konstanten Zeiger.
  uint8_t buf[MAX_DP_LENGTH];
  memcpy(buf, data, length);
  CbArg* arg = new CbArg(this, std::move(on_data), std::move(on_error));
  if (_optolink->write(address, length, buf, reinterpret_cast<void*>(arg))) return true;
  ESP_LOGW(TAG, "write_raw %04X: Queue voll", address);
  delete arg;
  return false;
}

void VitoConnect::_onData(uint8_t* data, uint8_t len, void* arg) {
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  if (cbArg == nullptr) return;
  if (cbArg->dp) {
    cbArg->dp->decode(data, len, cbArg->dp);
  } else if (cbArg->raw_data) {
    cbArg->raw_data(data, len);
  }
  delete cbArg;
}

void VitoConnect::_onError(uint8_t error, void* arg) {
  ESP_LOGD(TAG, "Error received: %d", error);
  CbArg* cbArg = reinterpret_cast<CbArg*>(arg);
  if (cbArg == nullptr) return;
  if (cbArg->dp) {
    if (cbArg->v->_onErrorCb) cbArg->v->_onErrorCb(error, cbArg->dp);
  } else if (cbArg->raw_error) {
    cbArg->raw_error(error);
  }
  delete cbArg;
}

}  // namespace vitoconnect
}  // namespace esphome
