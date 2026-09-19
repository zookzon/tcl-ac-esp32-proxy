/**
* Create by Miguel Ángel López on 20/07/19
* and modify by xaxexa
* Refactoring & component making:
* Соловей с паяльником 15.03.2024
**/
#include "esphome.h"
#include "esphome/core/defines.h"
#include "esphome/components/api/api_server.h"
#include "tclac.h"
#include <time.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <limits>
#include "driver/uart.h"

namespace esphome{
namespace tclac{

void tclacClimate::set_humidity_entity_id(const std::string &entity_id) {
	if (this->humidity_entity_id_ == entity_id)
		return;

	this->humidity_entity_id_ = entity_id;
	if (entity_id.empty()) {
		ESP_LOGI("TCL", "Home Assistant humidity source disabled");
		this->clear_external_humidity();
		return;
	}

	const std::string selected_entity = entity_id;
	ESP_LOGI("TCL", "Subscribing to Home Assistant humidity source: %s", entity_id.c_str());
	api::global_api_server->subscribe_home_assistant_state(
		entity_id, optional<std::string>(),
		[this, selected_entity](StringRef state) {
			// Old subscriptions can remain after the input is changed. Ignore
			// their callbacks and accept values only from the current selection.
			if (this->humidity_entity_id_ != selected_entity)
				return;

			auto humidity = parse_number<float>(state.c_str());
			if (!humidity.has_value() || !std::isfinite(*humidity) ||
			    *humidity < 0.0f || *humidity > 100.0f) {
				ESP_LOGW("TCL", "Invalid humidity state '%s' from %s",
				         state.c_str(), selected_entity.c_str());
				this->clear_external_humidity();
				return;
			}

			this->set_external_humidity(*humidity);
		});
}


ClimateTraits tclacClimate::traits() {
	auto traits = climate::ClimateTraits();

	
	//traits.set_supports_action(false);
	//traits.set_supports_current_temperature(true);
	//traits.set_supports_two_point_target_temperature(false);

	traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE); // Предудущие методы запрещены, теперь нужно использовать add_feature_flags
	traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
	traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);

	// Explicitly advertise the target-temperature grid to Home Assistant.
	// Without this, the custom traits() override can fall back to a 0.5 C UI step
	// even when YAML visual.target_temperature is configured as 1 C.
	traits.set_visual_min_temperature(16.0f);
	traits.set_visual_max_temperature(31.0f);
	traits.set_visual_target_temperature_step(this->target_temperature_step_);
	traits.set_visual_current_temperature_step(0.1f);

	traits.set_supported_modes(this->supported_modes_);
	if (this->preset_feature_enabled_) traits.set_supported_presets(this->supported_presets_);
	traits.set_supported_fan_modes(this->supported_fan_modes_);
	if (this->swing_feature_enabled_) traits.set_supported_swing_modes(this->supported_swing_modes_);
	
	traits.add_supported_mode(climate::CLIMATE_MODE_OFF);			// Выключенный режим кондиционера доступен всегда
	traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);			// Автоматический режим кондиционера тоже
	traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);		// Автоматический режим вентилятора доступен всегда
	if (this->swing_feature_enabled_) traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
	if (this->preset_feature_enabled_) traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);

	return traits;
}


void tclacClimate::setup() {
	if (this->a5_ack_only_) {
		this->dual_phase_fast_ = false;
		ESP_LOGI("TCL-A5", "A5 production takeover backend: 115200 8N1; HA climate control enabled; legacy BB disabled");
		ESP_LOGI("TCL-A5", "A5 dual-owner mode: AC=GPIO3/4, Factory Dongle TX=GPIO5 -> ESP RX, ESP TX=GPIO6 -> Dongle RX");
		return;
	}
	this->dual_phase_boot_ms_ = millis();
	this->dual_phase_fast_ = this->proxy_mode_ && this->dongle_uart_ != nullptr;
	ESP_LOGI("TCL-DUAL", "Phase 1 START: transparent boot bridge 115200 8N1; local ESPHome TX blocked for 12s");
	this->proxy_last_activity_ms_ = millis();
	this->proxy_last_dongle_activity_ms_ = millis();
	this->proxy_last_fallback_poll_ms_ = millis();

#ifdef CONF_RX_LED
	this->rx_led_pin_->setup();
	this->rx_led_pin_->digital_write(false);
#endif
#ifdef CONF_TX_LED
	this->tx_led_pin_->setup();
	this->tx_led_pin_->digital_write(false);
#endif
}

void tclacClimate::loop() {
	if (this->a5_ack_only_) {
		this->a5_ack_loop_();
		return;
	}
	// TAC-PRO12PEC cold boot emits a structured A5 protocol at 115200 8N1.
	// Bridge it transparently first, then switch both hardware UARTs to the
	// known normal-control BB protocol at 9600 8E1.
	if (this->dual_phase_fast_ && (millis() - this->dual_phase_boot_ms_) >= 12000U) {
		uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(50));
		uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(50));
		uart_flush_input(UART_NUM_0);
		uart_flush_input(UART_NUM_1);
		uart_set_baudrate(UART_NUM_0, 9600);
		uart_set_baudrate(UART_NUM_1, 9600);
		uart_set_word_length(UART_NUM_0, UART_DATA_8_BITS);
		uart_set_word_length(UART_NUM_1, UART_DATA_8_BITS);
		uart_set_parity(UART_NUM_0, UART_PARITY_EVEN);
		uart_set_parity(UART_NUM_1, UART_PARITY_EVEN);
		uart_set_stop_bits(UART_NUM_0, UART_STOP_BITS_1);
		uart_set_stop_bits(UART_NUM_1, UART_STOP_BITS_1);
		rx_pos_ = 0;
		rx_expected_len_ = 0;
		this->dongle_frame_pos_ = 0;
		this->dongle_frame_expected_len_ = 0;
		this->pending_tx_valid_ = false;
		this->pending_tx_len_ = 0;
		this->dual_phase_fast_ = false;
		this->proxy_last_activity_ms_ = millis();
		this->proxy_last_dongle_activity_ms_ = millis();
		this->proxy_last_fallback_poll_ms_ = millis();
		ESP_LOGI("TCL-DUAL", "Phase 2 START: switched both UARTs to 9600 8E1; BB parser/control enabled");
	}

	if (this->proxy_mode_ && this->dongle_uart_ != nullptr) {
		this->proxy_forward_dongle_to_ac_();
	}
	// Non-blocking bounded receiver. Known TCL status frames are 61, 65,
	// or 68 bytes. The total frame length is dataRX[4] + 6, including the
	// five-byte header and the trailing checksum byte.
	if (rx_pos_ > 0 && (millis() - rx_last_byte_ms_) > 250) {
		ESP_LOGW("TCL", "RX frame timeout; discarding %u bytes", static_cast<unsigned>(rx_pos_));
		rx_pos_ = 0;
		rx_expected_len_ = 0;
		dataShow(0, false);
	}

	while (esphome::uart::UARTDevice::available() > 0) {
		uint8_t value;
		if (!esphome::uart::UARTDevice::read_byte(&value))
			return;

		if (this->proxy_mode_ && this->dongle_uart_ != nullptr) {
			this->dongle_uart_->write_byte(value);
			this->proxy_last_activity_ms_ = millis();
		}

		if (this->dual_phase_fast_)
			continue;

		if (rx_pos_ == 0) {
			if (value != 0xBB)
				continue;
			dataShow(0, true);
		}

		if (rx_pos_ >= sizeof(dataRX)) {
			ESP_LOGW("TCL", "RX overflow prevented");
			rx_pos_ = 0;
			rx_expected_len_ = 0;
			dataShow(0, false);
			continue;
		}

		dataRX[rx_pos_++] = value;
		rx_last_byte_ms_ = millis();

		if (rx_pos_ == 5) {
			rx_expected_len_ = static_cast<size_t>(dataRX[4]) + 6U;
			const bool supported_length =
				rx_expected_len_ == 51U ||
				rx_expected_len_ == 61U ||
				rx_expected_len_ == 65U ||
				rx_expected_len_ == 68U;
			if (!supported_length || rx_expected_len_ > sizeof(dataRX)) {
				ESP_LOGW("TCL", "Unsupported RX frame length %u",
				         static_cast<unsigned>(rx_expected_len_));
				rx_pos_ = 0;
				rx_expected_len_ = 0;
				dataShow(0, false);
			}
			continue;
		}

		if (rx_expected_len_ != 0 && rx_pos_ == rx_expected_len_) {
			const byte check = getChecksum(dataRX, rx_expected_len_);
			if (check == dataRX[rx_expected_len_ - 1]) {
				readData();
			} else {
				ESP_LOGW("TCL", "Invalid RX checksum: calculated %02X, received %02X",
				         check, dataRX[rx_expected_len_ - 1]);
			}
			rx_pos_ = 0;
			rx_expected_len_ = 0;
			dataShow(0, false);
		}
	}

	// Energy probe: send CMD 0x0A about 180 ms after a completed state
	// response. This is deliberately non-blocking and rate-limited.
	if (this->energy_probe_pending_ && static_cast<int32_t>(millis() - this->energy_probe_due_ms_) >= 0) {
		this->energy_probe_pending_ = false;
		this->energy_probe_last_query_ms_ = millis();
		ESP_LOGI("TCL-ENERGY", "TX CMD 0x0A energy probe: BB 00 01 0A 03 05 00 00 B6");
		this->esphome::uart::UARTDevice::write_array(this->energy_probe_query_, sizeof(this->energy_probe_query_));
	}

	if (this->proxy_mode_ && this->dongle_uart_ != nullptr) {
		this->proxy_try_send_pending_();
	}
}

void tclacClimate::update() {
	if (this->a5_ack_only_) return;
	if (this->proxy_mode_ && this->dongle_uart_ != nullptr) {
		if (this->dual_phase_fast_) return;
		const uint32_t now = millis();
		// Normally the factory dongle owns polling. Only emit our own status poll
		// after the dongle has been quiet for a long time, so the original app
		// remains the primary bus master while connected.
		if ((now - this->proxy_last_dongle_activity_ms_) < this->proxy_fallback_poll_interval_ms_)
			return;
		if ((now - this->proxy_last_fallback_poll_ms_) < this->proxy_fallback_poll_interval_ms_)
			return;
		this->proxy_last_fallback_poll_ms_ = now;
		this->proxy_queue_or_send_(poll, sizeof(poll));
		return;
	}

	tclacClimate::dataShow(1,1);
	this->esphome::uart::UARTDevice::write_array(poll, sizeof(poll));
	tclacClimate::dataShow(1,0);
}

void tclacClimate::readData() {
	// Log every checksum-valid frame before dispatching by command.
	auto frame_raw = tclacClimate::getHex(dataRX, static_cast<byte>(rx_expected_len_));
	ESP_LOGD("TCL", "Valid RX[%u]: %s", static_cast<unsigned>(rx_expected_len_), frame_raw.c_str());

	// CMD 0x0A is a 45-byte payload / 51-byte total energy response.
	// Probe only: keep the entire response in logs and do not publish an HA
	// entity until TAC-PRO12PEC's counter layout is verified from real data.
	if (dataRX[3] == 0x0A) {
		ESP_LOGI("TCL-ENERGY", "RX CMD 0x0A energy response [%u]: %s",
		         static_cast<unsigned>(rx_expected_len_), frame_raw.c_str());
		if (rx_expected_len_ >= 51U) {
			const uint8_t flag = dataRX[7];
			ESP_LOGI("TCL-ENERGY", "Energy payload marker=0x%02X data_flag=0x%02X", dataRX[5], flag);

			// Index every byte in the 45-byte CMD 0x0A payload.  Index P00 is
			// dataRX[5], so the observed subtype/flag is P02/dataRX[7].
			String indexed;
			for (size_t i = 5; i <= 49; i++) {
				char item[16];
				snprintf(item, sizeof(item), "P%02u=%02X%s",
				         static_cast<unsigned>(i - 5), dataRX[i],
				         ((i - 5) % 8 == 7 || i == 49) ? "" : " ");
				indexed += item;
				if ((i - 5) % 8 == 7 || i == 49) {
					ESP_LOGI("TCL-ENERGY-RAW", "%s", indexed.c_str());
					indexed = "";
				}
			}

			ESP_LOGI("TCL-ENERGY-STATE",
			         "snapshot B38=%u B39=%u (%.1fA) B40=0x%02X B45=%u B46=%u",
			         this->energy_state_b38_, this->energy_state_b39_,
			         static_cast<float>(this->energy_state_b39_) / 10.0f,
			         this->energy_state_b40_, this->energy_state_b45_, this->energy_state_b46_);

			const uint32_t now = millis();
			if (this->energy_prev_payload_valid_) {
				String changed;
				unsigned changes = 0;
				for (size_t pidx = 0; pidx < 45; pidx++) {
					const uint8_t cur = dataRX[5 + pidx];
					const uint8_t prev = this->energy_prev_payload_[pidx];
					if (cur == prev) continue;
					char item[28];
					snprintf(item, sizeof(item), "P%02u:%02X>%02X ",
					         static_cast<unsigned>(pidx), prev, cur);
					changed += item;
					changes++;
				}
				ESP_LOGI("TCL-ENERGY-DELTA", "dt=%ums changed=%u %s",
				         static_cast<unsigned>(now - this->energy_prev_rx_ms_), changes,
				         changes ? changed.c_str() : "(no payload changes)");
			} else {
				ESP_LOGI("TCL-ENERGY-DELTA", "first subtype 0x%02X sample; baseline stored", flag);
			}

			for (size_t pidx = 0; pidx < 45; pidx++)
				this->energy_prev_payload_[pidx] = dataRX[5 + pidx];
			this->energy_prev_payload_valid_ = true;
			this->energy_prev_rx_ms_ = now;

			// Deliberately do NOT decode subtype 0x0D as kWh here. Previous builds
			// applied the documented 0x0C BCD layout as a candidate; this build
			// returns to raw evidence until the TAC-PRO12PEC 0x0D layout is proven.
			ESP_LOGI("TCL-ENERGY", "Subtype 0x%02X kept RAW; no kWh interpretation applied", flag);
		}
		return;
	}

	// This decoder below is for full state responses only. Ignore other
	// command families rather than interpreting their payload as status bytes.
	if (dataRX[3] != 0x03 && dataRX[3] != 0x04) {
		ESP_LOGD("TCL", "Unhandled valid CMD 0x%02X (%u bytes)", dataRX[3],
		         static_cast<unsigned>(rx_expected_len_));
		return;
	}

	// Chain an energy probe onto a normal GET response, as observed on TCL
	// factory-module traffic. Rate limit to one probe every 30 seconds.
	if (dataRX[3] == 0x04 &&
	    (this->energy_probe_last_query_ms_ == 0 ||
	     (millis() - this->energy_probe_last_query_ms_) >= this->energy_probe_interval_ms_)) {
		this->energy_probe_due_ms_ = millis() + 180U;
		this->energy_probe_pending_ = true;
		ESP_LOGD("TCL-ENERGY", "CMD 0x04 state received; scheduling CMD 0x0A in 180 ms");
	}

	// HEAT transaction diagnostic: correlate the exact SET frame with the next
	// valid CMD 0x03/0x04 reply. Logging only; no mode/control bytes are changed.
	if (this->heat_diag_pending_) {
		const uint32_t heat_dt = millis() - this->heat_diag_tx_ms_;
		ESP_LOGI("TCL-HEAT",
		         "RX after HEAT dt=%ums CMD=0x%02X len=%u status_mode_raw=0x%02X mode_mask=0x%02X power_bit4=%u frame=%s",
		         static_cast<unsigned>(heat_dt), dataRX[3],
		         static_cast<unsigned>(rx_expected_len_), dataRX[MODE_POS],
		         static_cast<unsigned>(dataRX[MODE_POS] & MODE_MASK),
		         static_cast<unsigned>((dataRX[MODE_POS] >> 4) & 0x01), frame_raw.c_str());
		// Keep the correlation window open long enough to capture the immediate
		// reply plus several normal status polls. It expires automatically.
		if (heat_dt >= 15000U) {
			ESP_LOGI("TCL-HEAT", "HEAT diagnostic window complete");
			this->heat_diag_pending_ = false;
		}
	}

	// Only publish diagnostic sensors when their state changes. This keeps the
	// five-second status polling intact while avoiding duplicate API traffic.
	auto publish_if_changed = [](sensor::Sensor *target, float value) {
		if (target == nullptr)
			return;
		const bool value_nan = std::isnan(value);
		const bool state_nan = target->has_state() && std::isnan(target->state);
		if (!target->has_state() || value_nan != state_nan || (!value_nan && target->state != value))
			target->publish_state(value);
	};

	// Experimental read-only diagnostics from ElectApp/TCLAirConditioner.
	// Byte 31 was removed: TAC-PRO12PEC consistently reports 0xFF there.
	const uint8_t error_code = dataRX[16];
	const uint8_t indoor_coil_raw = dataRX[30];
	const float indoor_coil_temperature =
		(static_cast<float>(static_cast<uint16_t>(indoor_coil_raw) << 8U) / 374.0f - 32.0f) / 1.8f;
	const float unavailable = std::numeric_limits<float>::quiet_NaN();

	publish_if_changed(this->error_code_sensor_, error_code);
	publish_if_changed(this->indoor_coil_temperature_sensor_,
	                   indoor_coil_raw == 0xFF ? unavailable : indoor_coil_temperature);
	publish_if_changed(this->indoor_coil_raw_sensor_, indoor_coil_raw);

	// Experimental read-only diagnostics from SkateWarp/ESPHome-Airmax.
	// Values are exposed for validation only and never influence control frames.
	const float pipe_out_temperature = static_cast<float>(static_cast<int>(dataRX[35]) - 32);
	const float pipe_in_temperature = static_cast<float>(static_cast<int>(dataRX[36]) - 32);
	// Passive Probe v2. A recent third-party fork proposes an offset of 22 for
	// bytes 35-37. These names and formulas are deliberately kept as disabled
	// candidates; they do not replace the Airmax interpretations above.
	const float outdoor_ambient_candidate =
		dataRX[35] == 0xFF ? unavailable : static_cast<float>(static_cast<int>(dataRX[35]) - 22);
	const float outdoor_exhaust_candidate =
		dataRX[36] == 0xFF ? unavailable : static_cast<float>(static_cast<int>(dataRX[36]) - 22);
	const float outdoor_condenser_candidate =
		dataRX[37] == 0xFF ? unavailable : static_cast<float>(static_cast<int>(dataRX[37]) - 22);
	const float compressor_frequency_candidate =
		dataRX[38] == 0xFF ? unavailable : static_cast<float>(dataRX[38]);
	// Bytes 37 and 38 vary with operating conditions, but no reliable public
	// mapping or conversion to external-unit temperature is available for this
	// model. Publish the untouched values as disabled diagnostic probes only.
	const uint8_t external_temperature_candidate_raw_37 = dataRX[37];
	const uint8_t external_temperature_candidate_raw_38 = dataRX[38];
	const float compressor_current = static_cast<float>(dataRX[39]) / 10.0f;
	const uint8_t compressor_state = dataRX[40];
	const uint8_t airmax_fault_code = dataRX[44];
	const uint8_t raw_byte_45 = dataRX[45];
	const uint8_t outside_motor = dataRX[46];

	// Context snapshot for the next CMD 0x0A probe only.
	this->energy_state_b38_ = dataRX[38];
	this->energy_state_b39_ = dataRX[39];
	this->energy_state_b40_ = dataRX[40];
	this->energy_state_b45_ = dataRX[45];
	this->energy_state_b46_ = dataRX[46];
	const uint8_t vertical_vane_position = dataRX[51];
	const uint8_t horizontal_vane_position = dataRX[52];

	publish_if_changed(this->pipe_out_temperature_sensor_, pipe_out_temperature);
	publish_if_changed(this->pipe_in_temperature_sensor_, pipe_in_temperature);
	publish_if_changed(this->outdoor_ambient_candidate_sensor_, outdoor_ambient_candidate);
	publish_if_changed(this->outdoor_exhaust_candidate_sensor_, outdoor_exhaust_candidate);
	publish_if_changed(this->outdoor_condenser_candidate_sensor_, outdoor_condenser_candidate);
	publish_if_changed(this->compressor_frequency_candidate_sensor_, compressor_frequency_candidate);
	publish_if_changed(this->external_temperature_candidate_raw_37_sensor_,
	                   external_temperature_candidate_raw_37);
	publish_if_changed(this->external_temperature_candidate_raw_38_sensor_,
	                   external_temperature_candidate_raw_38);
	publish_if_changed(this->compressor_current_sensor_, compressor_current);
	publish_if_changed(this->compressor_state_sensor_, compressor_state);
	publish_if_changed(this->airmax_fault_code_sensor_, airmax_fault_code);
	publish_if_changed(this->raw_byte_45_sensor_, raw_byte_45);
	const float active_supply_voltage = raw_byte_45 >= 180
		? static_cast<float>(raw_byte_45)
		: unavailable;
	publish_if_changed(this->active_supply_voltage_sensor_, active_supply_voltage);
	publish_if_changed(this->outside_motor_sensor_, outside_motor);
	publish_if_changed(this->vertical_vane_position_sensor_, vertical_vane_position);
	publish_if_changed(this->horizontal_vane_position_sensor_, horizontal_vane_position);

	// Operation-state probe: log only when a machine-side operating field changes.
	// IMPORTANT: this is diagnostic-only. It does not use or modify Climate Action,
	// target temperature, room temperature, or any control decision.
	static bool op_probe_initialized = false;
	static uint8_t prev_b38 = 0;
	static uint8_t prev_b40 = 0;
	static uint8_t prev_b46 = 0;
	static uint8_t prev_current_raw = 0;
	const uint8_t current_raw = dataRX[39];
	if (!op_probe_initialized || dataRX[38] != prev_b38 ||
	    compressor_state != prev_b40 || outside_motor != prev_b46 ||
	    current_raw != prev_current_raw) {
		ESP_LOGI("TCL-OP",
		         "Machine change: power=%s mode_raw=0x%02X B35=%u B36=%u B37=%u B38=%u B39_current_raw=%u (%.1fA) B40=0x%02X B45=%u B46=%u",
		         (dataRX[7] & 0x01) ? "ON" : "OFF", dataRX[8],
		         dataRX[35], dataRX[36], dataRX[37], dataRX[38],
		         current_raw, compressor_current, compressor_state, raw_byte_45,
		         outside_motor);
		prev_b38 = dataRX[38];
		prev_b40 = compressor_state;
		prev_b46 = outside_motor;
		prev_current_raw = current_raw;
		op_probe_initialized = true;
	}

	// Always-on diagnostics for protocol validation. These values are intentionally
	// kept out of Home Assistant and are available in the ESPHome log only.
	ESP_LOGD("TCL", "Diagnostics: error=0x%02X, indoor_raw=%u (%.2f C), pipe_out=%.1f C, pipe_in=%.1f C, outdoor_candidates=%.1f/%.1f/%.1f C, frequency_candidate=%.0f Hz, ext_candidate_b37=%u, ext_candidate_b38=%u, current=%.1f A, compressor=0x%02X, fault=0x%02X, raw_b45=%u, outside_motor=%u, vane_v=%u, vane_h=%u",
	         error_code, indoor_coil_raw, indoor_coil_temperature,
	         pipe_out_temperature, pipe_in_temperature,
	         outdoor_ambient_candidate, outdoor_exhaust_candidate,
	         outdoor_condenser_candidate, compressor_frequency_candidate,
	         external_temperature_candidate_raw_37,
	         external_temperature_candidate_raw_38, compressor_current,
	         compressor_state, airmax_fault_code, raw_byte_45, outside_motor,
	         vertical_vane_position, horizontal_vane_position);
	const uint16_t raw_room_temperature =
		(static_cast<uint16_t>(dataRX[17]) << 8U) | dataRX[18];
	const float decoded_room_temperature =
		(static_cast<float>(raw_room_temperature) / 374.0f - 32.0f) / 1.8f;
	current_temperature = std::round(decoded_room_temperature * 10.0f) / 10.0f;
	publish_if_changed(this->room_temperature_sensor_, current_temperature);
	target_temperature = (dataRX[FAN_SPEED_POS] & SET_TEMP_MASK) + 16;

	//ESP_LOGD("TCL", "TEMP: %f ", current_temperature);

	if (dataRX[MODE_POS] & ( 1 << 4)) {
		// Если кондиционер включен, то разбираем данные для отображения
		// ESP_LOGD("TCL", "AC is on");
		uint8_t modeswitch = MODE_MASK & dataRX[MODE_POS];
		uint8_t fanspeedswitch = FAN_SPEED_MASK & dataRX[FAN_SPEED_POS];
		uint8_t swingmodeswitch = SWING_MODE_MASK & dataRX[SWING_POS];

		switch (modeswitch) {
			case MODE_AUTO:
				mode = climate::CLIMATE_MODE_AUTO;
				break;
			case MODE_COOL:
				mode = climate::CLIMATE_MODE_COOL;
				break;
			case MODE_DRY:
				mode = climate::CLIMATE_MODE_DRY;
				break;
			case MODE_FAN_ONLY:
				mode = climate::CLIMATE_MODE_FAN_ONLY;
				break;
			case MODE_HEAT:
				mode = climate::CLIMATE_MODE_HEAT;
				break;
			default:
				mode = climate::CLIMATE_MODE_AUTO;
		}

		if ( dataRX[FAN_QUIET_POS] & FAN_QUIET) {
			fan_mode = climate::CLIMATE_FAN_QUIET;
		} else if (dataRX[MODE_POS] & FAN_DIFFUSE){
			fan_mode = climate::CLIMATE_FAN_DIFFUSE;
		} else {
			switch (fanspeedswitch) {
				case FAN_AUTO:
					fan_mode = climate::CLIMATE_FAN_AUTO;
					break;
				case FAN_LOW:
					fan_mode = climate::CLIMATE_FAN_LOW;
					break;
				case FAN_MIDDLE:
					fan_mode = climate::CLIMATE_FAN_MIDDLE;
					break;
				case FAN_MEDIUM:
					fan_mode = climate::CLIMATE_FAN_MEDIUM;
					break;
				case FAN_HIGH:
					fan_mode = climate::CLIMATE_FAN_HIGH;
					break;
				case FAN_FOCUS:
					fan_mode = climate::CLIMATE_FAN_FOCUS;
					break;
				default:
					fan_mode = climate::CLIMATE_FAN_AUTO;
			}
		}

		switch (swingmodeswitch) {
			case SWING_OFF: 
				swing_mode = climate::CLIMATE_SWING_OFF;
				break;
			case SWING_HORIZONTAL:
				swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
				break;
			case SWING_VERTICAL:
				swing_mode = climate::CLIMATE_SWING_VERTICAL;
				break;
			case SWING_BOTH:
				swing_mode = climate::CLIMATE_SWING_BOTH;
				break;
		}
		
		// Обработка данных о пресете
		preset = ClimatePreset::CLIMATE_PRESET_NONE;
		if (dataRX[7] & (1 << 6)){
			preset = ClimatePreset::CLIMATE_PRESET_ECO;
		} else if (dataRX[9] & (1 << 2)){
			preset = ClimatePreset::CLIMATE_PRESET_COMFORT;
		} else if (dataRX[19] & (1 << 0)){
			preset = ClimatePreset::CLIMATE_PRESET_SLEEP;
		}
		
	} else {
		// Если кондиционер выключен, то все режимы показываются, как выключенные
		mode = climate::CLIMATE_MODE_OFF;
		//fan_mode = climate::CLIMATE_FAN_OFF;
		swing_mode = climate::CLIMATE_SWING_OFF;
		preset = ClimatePreset::CLIMATE_PRESET_NONE;
	}

	// Action v2: derive compressor operation from machine-side status bytes.
	// Evidence from captured TCL status packets:
	//   running-like: B38>0, B39>0, B40=0x8A, B46>0
	//   stopped-like: B38=0, B39=0, B40=0x80, B46=0
	// B40 is the primary state indicator. B38/B39/B46 provide supporting
	// evidence so one field changing briefly does not make Action flicker.
	// This intentionally does NOT use room/target-temperature comparisons.
	const bool machine_running_state = compressor_state == 0x8A;
	const bool machine_activity_evidence =
		dataRX[38] > 0 || dataRX[39] > 0 || outside_motor > 0;
	const bool machine_stopped_state =
		compressor_state == 0x80 && dataRX[38] == 0 &&
		dataRX[39] == 0 && outside_motor == 0;
	const bool compressor_active =
		machine_running_state && machine_activity_evidence && !machine_stopped_state;

	switch (mode) {
		case climate::CLIMATE_MODE_COOL:
			action = compressor_active
				? climate::CLIMATE_ACTION_COOLING
				: climate::CLIMATE_ACTION_IDLE;
			break;
		case climate::CLIMATE_MODE_HEAT:
			action = compressor_active
				? climate::CLIMATE_ACTION_HEATING
				: climate::CLIMATE_ACTION_IDLE;
			break;
		case climate::CLIMATE_MODE_DRY:
			action = compressor_active
				? climate::CLIMATE_ACTION_DRYING
				: climate::CLIMATE_ACTION_IDLE;
			break;
		case climate::CLIMATE_MODE_AUTO:
			// The captured machine-side fields prove compressor activity but do not
			// yet identify AUTO cooling vs heating direction. Keep AUTO conservative
			// instead of reintroducing target/current-temperature inference.
			action = climate::CLIMATE_ACTION_IDLE;
			break;
		case climate::CLIMATE_MODE_FAN_ONLY:
			action = climate::CLIMATE_ACTION_FAN;
			break;
		case climate::CLIMATE_MODE_OFF:
		default:
			action = climate::CLIMATE_ACTION_OFF;
			break;
	}

	// Публикуем данные
	this->publish_state();
	allow_take_control = true;
   }

// Climate control
void tclacClimate::control(const ClimateCall &call) {
	if (this->a5_ack_only_) {
		// Production A5 backend. Commands are absolute and state is NOT optimistic:
		// the following 0C0C report remains the authority for Home Assistant.
		if (call.get_mode().has_value()) {
			auto m = call.get_mode().value();
			if (m == climate::CLIMATE_MODE_OFF) {
				this->a5_send_test_power_(false);
			} else {
				uint8_t raw = 0;
				switch (m) {
					case climate::CLIMATE_MODE_AUTO: raw=0; break;
					case climate::CLIMATE_MODE_COOL: raw=1; break;
					case climate::CLIMATE_MODE_DRY: raw=2; break;
					case climate::CLIMATE_MODE_FAN_ONLY: raw=3; break;
					case climate::CLIMATE_MODE_HEAT: raw=4; break;
					default: ESP_LOGW("TCL-A5", "Unsupported HA mode"); return;
				}
				// The repo uses an absolute power-on record together with mode when
				// power is off/unknown. It is harmless if already on.
				uint8_t ctr=this->a5_command_counter_++;
				uint8_t f[18]={0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,
					0x00,0x01,0x01, 0x00,0x12,raw};
				this->a5_send_frame_(f,sizeof(f),"CLIMATE mode/power");
				this->a5_commands_sent_++;
			}
		}
		if (call.get_target_temperature().has_value())
			this->a5_send_setpoint_c_(call.get_target_temperature().value());
		if (call.get_fan_mode().has_value()) {
			uint8_t speed=0;
			switch (call.get_fan_mode().value()) {
				case climate::CLIMATE_FAN_AUTO: speed=0; break;
				case climate::CLIMATE_FAN_QUIET: speed=1; break;
				case climate::CLIMATE_FAN_LOW: speed=2; break;
				case climate::CLIMATE_FAN_MIDDLE: speed=3; break;
				case climate::CLIMATE_FAN_MEDIUM: speed=4; break;
				case climate::CLIMATE_FAN_HIGH: speed=5; break;
				case climate::CLIMATE_FAN_FOCUS: speed=6; break;
				case climate::CLIMATE_FAN_DIFFUSE: speed=7; break;
				default: speed=0; break;
			}
			this->a5_send_fan_(speed);
		}
		if (this->swing_feature_enabled_ && call.get_swing_mode().has_value())
			this->a5_send_swing_(call.get_swing_mode().value());
		if (this->preset_feature_enabled_ && call.get_preset().has_value())
			this->a5_send_preset_(call.get_preset().value());
		return;
	}
	// Запрашиваем данные из переключателя режимов работы кондиционера
	if (call.get_mode().has_value()){
		switch_climate_mode = call.get_mode().value();
		ESP_LOGD("TCL", "Get MODE from call");
	} else {
		switch_climate_mode = mode;
		ESP_LOGD("TCL", "Get MODE from AC");
	}
	
	// Запрашиваем данные из переключателя предустановок кондиционера
	if (call.get_preset().has_value()){
		switch_preset = call.get_preset().value();
	} else {
		switch_preset = preset.value();
	}
	
	// Запрашиваем данные из переключателя режимов вентилятора
	if (call.get_fan_mode().has_value()){
		switch_fan_mode = call.get_fan_mode().value();
	} else {
		switch_fan_mode = fan_mode.value();
	}
	
	// Запрашиваем данные из переключателя режимов качания заслонок
	if (call.get_swing_mode().has_value()){
		switch_swing_mode = call.get_swing_mode().value();
	} else {
		// А если в переключателе пусто- заполняем значением из последнего опроса состояния. Типа, ничего не поменялось.
		switch_swing_mode = swing_mode;
	}
	
	// Расчет температуры
	if (call.get_target_temperature().has_value()) {
		target_temperature_set = 31-(int)call.get_target_temperature().value();
	} else {
		target_temperature_set = 31-(int)target_temperature;
	}
	
	is_call_control = true;
	takeControl();
	allow_take_control = true;
}
	
	
void tclacClimate::takeControl() {
	// Start every command from a fully initialized frame. Upstream left byte 29
	// and several other positions dependent on stale RAM contents.
	std::memset(dataTX, 0, sizeof(dataTX));
	
	if (is_call_control != true){
		ESP_LOGD("TCL", "Get MODE from AC for force config");
		switch_climate_mode = mode;
		switch_preset = preset.has_value()
			? preset.value()
			: ClimatePreset::CLIMATE_PRESET_NONE;
		switch_fan_mode = fan_mode.has_value()
			? fan_mode.value()
			: climate::CLIMATE_FAN_AUTO;
		switch_swing_mode = swing_mode;
		target_temperature_set = 31-(int)target_temperature;
	}
	
	// Включаем или отключаем пищалку в зависимости от переключателя в настройках
	if (beeper_status_){
		ESP_LOGD("TCL", "Beep mode ON");
		dataTX[7] += 0b00100000;
	} else {
		ESP_LOGD("TCL", "Beep mode OFF");
		dataTX[7] += 0b00000000;
	}
	
	// Включаем или отключаем дисплей на кондиционере в зависимости от переключателя в настройках
	// Включаем дисплей только если кондиционер в одном из рабочих режимов
	
	// ВНИМАНИЕ! При выключении дисплея кондиционер сам принудительно переходит в автоматический режим!
	
	if ((display_status_) && (switch_climate_mode != climate::CLIMATE_MODE_OFF)){
		ESP_LOGD("TCL", "Dispaly turn ON");
		dataTX[7] += 0b01000000;
	} else {
		ESP_LOGD("TCL", "Dispaly turn OFF");
		dataTX[7] += 0b00000000;
	}
		
	// Настраиваем режим работы кондиционера
	switch (switch_climate_mode) {
		case climate::CLIMATE_MODE_OFF:
			dataTX[7] += 0b00000000;
			dataTX[8] += 0b00000000;
			break;
		case climate::CLIMATE_MODE_AUTO:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00001000;
			break;
		case climate::CLIMATE_MODE_COOL:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000011;	
			break;
		case climate::CLIMATE_MODE_DRY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000010;	
			break;
		case climate::CLIMATE_MODE_FAN_ONLY:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000111;	
			break;
		case climate::CLIMATE_MODE_HEAT:
			dataTX[7] += 0b00000100;
			dataTX[8] += 0b00000001;	
			break;
		default:
			ESP_LOGW("TCL", "Unsupported climate mode value %u; control command rejected",
			         static_cast<unsigned>(switch_climate_mode));
			allow_take_control = false;
			is_call_control = false;
			return;
	}

	// Настраиваем режим вентилятора
	switch(switch_fan_mode) {
		case climate::CLIMATE_FAN_AUTO:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000000;
			break;
		case climate::CLIMATE_FAN_QUIET:
			dataTX[8]	+= 0b10000000;
			dataTX[10]	+= 0b00000000;
			break;
		case climate::CLIMATE_FAN_LOW:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000001;
			break;
		case climate::CLIMATE_FAN_MIDDLE:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000110;
			break;
		case climate::CLIMATE_FAN_MEDIUM:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000011;
			break;
		case climate::CLIMATE_FAN_HIGH:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000111;
			break;
		case climate::CLIMATE_FAN_FOCUS:
			dataTX[8]	+= 0b00000000;
			dataTX[10]	+= 0b00000101;
			break;
		case climate::CLIMATE_FAN_DIFFUSE:
			dataTX[8]	+= 0b01000000;
			dataTX[10]	+= 0b00000000;
			break;
		default:
			ESP_LOGW("TCL", "Unsupported fan mode value %u; control command rejected",
			         static_cast<unsigned>(switch_fan_mode));
			allow_take_control = false;
			is_call_control = false;
			return;
	}
	
	// Устанавливаем режим качания заслонок
	switch(switch_swing_mode) {
		case climate::CLIMATE_SWING_OFF:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_VERTICAL:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00000000;
			break;
		case climate::CLIMATE_SWING_HORIZONTAL:
			dataTX[10]	+= 0b00000000;
			dataTX[11]	+= 0b00001000;
			break;
		case climate::CLIMATE_SWING_BOTH:
			dataTX[10]	+= 0b00111000;
			dataTX[11]	+= 0b00001000;  
			break;
	}
	
	// Устанавливаем предустановки кондиционера
	switch(switch_preset) {
		case ClimatePreset::CLIMATE_PRESET_NONE:
			break;
		case ClimatePreset::CLIMATE_PRESET_ECO:
			dataTX[7]	+= 0b10000000;
			break;
		case ClimatePreset::CLIMATE_PRESET_SLEEP:
			dataTX[19]	+= 0b00000001;
			break;
		case ClimatePreset::CLIMATE_PRESET_COMFORT:
			dataTX[8]	+= 0b00010000;
			break;
		default:
			ESP_LOGW("TCL", "Unsupported preset value %u; control command rejected",
			         static_cast<unsigned>(switch_preset));
			allow_take_control = false;
			is_call_control = false;
			return;
	}

        //Режим заслонок
		//	Вертикальная заслонка
		//		Качание вертикальной заслонки [10 байт, маска 00111000]:
		//			000 - Качание отключено, заслонка в последней позиции или в фиксации
		//			111 - Качание включено в выбранном режиме
		//		Режим качания вертикальной заслонки (режим фиксации заслонки роли не играет, если качание включено) [32 байт, маска 00011000]:
		//			01 - качание сверху вниз, ПО УМОЛЧАНИЮ
		//			10 - качание в верхней половине
		//			11 - качание в нижней половине
		//		Режим фиксации заслонки (режим качания заслонки роли не играет, если качание выключено) [32 байт, маска 00000111]:
		//			000 - нет фиксации, ПО УМОЛЧАНИЮ
		//			001 - фиксация вверху
		//			010 - фиксация между верхом и серединой
		//			011 - фиксация в середине
		//			100 - фиксация между серединой и низом
		//			101 - фиксация внизу
		//	Горизонтальные заслонки
		//		Качание горизонтальных заслонок [11 байт, маска 00001000]:
		//			0 - Качание отключено, заслонки в последней позиции или в фиксации
		//			1 - Качание включено в выбранном режиме
		//		Режим качания горизонтальных заслонок (режим фиксации заслонок роли не играет, если качание включено) [33 байт, маска 00111000]:
		//			001 - качание слева направо, ПО УМОЛЧАНИЮ
		//			010 - качание слева
		//			011 - качание по середине
		//			100 - качание справа
		//		Режим фиксации горизонтальных заслонок (режим качания заслонок роли не играет, если качание выключено) [33 байт, маска 00000111]:
		//			000 - нет фиксации, ПО УМОЛЧАНИЮ
		//			001 - фиксация слева
		//			010 - фиксация между левой стороной и серединой
		//			011 - фиксация в середине
		//			100 - фиксация между серединой и правой стороной
		//			101 - фиксация справа
		
		
	// Устанавливаем режим для качания вертикальной заслонки
	switch(vertical_swing_direction_) {
		case VerticalSwingDirection::UP_DOWN:
			dataTX[32]	+= 0b00001000;
			ESP_LOGD("TCL", "Vertical swing: up-down");
			break;
		case VerticalSwingDirection::UPSIDE:
			dataTX[32]	+= 0b00010000;
			ESP_LOGD("TCL", "Vertical swing: upper");
			break;
		case VerticalSwingDirection::DOWNSIDE:
			dataTX[32]	+= 0b00011000;
			ESP_LOGD("TCL", "Vertical swing: downer");
			break;
	}
	// Устанавливаем режим для качания горизонтальных заслонок
	switch(horizontal_swing_direction_) {
		case HorizontalSwingDirection::LEFT_RIGHT:
			dataTX[33]	+= 0b00001000;
			ESP_LOGD("TCL", "Horizontal swing: left-right");
			break;
		case HorizontalSwingDirection::LEFTSIDE:
			dataTX[33]	+= 0b00010000;
			ESP_LOGD("TCL", "Horizontal swing: lefter");
			break;
		case HorizontalSwingDirection::CENTER:
			dataTX[33]	+= 0b00011000;
			ESP_LOGD("TCL", "Horizontal swing: center");
			break;
		case HorizontalSwingDirection::RIGHTSIDE:
			dataTX[33]	+= 0b00100000;
			ESP_LOGD("TCL", "Horizontal swing: righter");
			break;
	}
	// Устанавливаем положение фиксации вертикальной заслонки
	switch(vertical_direction_) {
		case AirflowVerticalDirection::LAST:
			dataTX[32]	+= 0b00000000;
			ESP_LOGD("TCL", "Vertical fix: last position");
			break;
		case AirflowVerticalDirection::MAX_UP:
			dataTX[32]	+= 0b00000001;
			ESP_LOGD("TCL", "Vertical fix: up");
			break;
		case AirflowVerticalDirection::UP:
			dataTX[32]	+= 0b00000010;
			ESP_LOGD("TCL", "Vertical fix: upper");
			break;
		case AirflowVerticalDirection::CENTER:
			dataTX[32]	+= 0b00000011;
			ESP_LOGD("TCL", "Vertical fix: center");
			break;
		case AirflowVerticalDirection::DOWN:
			dataTX[32]	+= 0b00000100;
			ESP_LOGD("TCL", "Vertical fix: downer");
			break;
		case AirflowVerticalDirection::MAX_DOWN:
			dataTX[32]	+= 0b00000101;
			ESP_LOGD("TCL", "Vertical fix: down");
			break;
	}
	// Устанавливаем положение фиксации горизонтальных заслонок
	switch(horizontal_direction_) {
		case AirflowHorizontalDirection::LAST:
			dataTX[33]	+= 0b00000000;
			ESP_LOGD("TCL", "Horizontal fix: last position");
			break;
		case AirflowHorizontalDirection::MAX_LEFT:
			dataTX[33]	+= 0b00000001;
			ESP_LOGD("TCL", "Horizontal fix: left");
			break;
		case AirflowHorizontalDirection::LEFT:
			dataTX[33]	+= 0b00000010;
			ESP_LOGD("TCL", "Horizontal fix: lefter");
			break;
		case AirflowHorizontalDirection::CENTER:
			dataTX[33]	+= 0b00000011;
			ESP_LOGD("TCL", "Horizontal fix: center");
			break;
		case AirflowHorizontalDirection::RIGHT:
			dataTX[33]	+= 0b00000100;
			ESP_LOGD("TCL", "Horizontal fix: righter");
			break;
		case AirflowHorizontalDirection::MAX_RIGHT:
			dataTX[33]	+= 0b00000101;
			ESP_LOGD("TCL", "Horizontal fix: right");
			break;
	}

	// Установка температуры
	dataTX[9] = target_temperature_set;
		
	// Собираем массив байт для отправки в кондиционер
	dataTX[0] = 0xBB;	//стартовый байт заголовка
	dataTX[1] = 0x00;	//стартовый байт заголовка
	dataTX[2] = 0x01;	//стартовый байт заголовка
	dataTX[3] = 0x03;	//0x03 - управление, 0x04 - опрос
	dataTX[4] = 0x20;	//0x20 - управление, 0x19 - опрос
	dataTX[5] = 0x03;	//??
	dataTX[6] = 0x01;	//??
	//dataTX[7] = 0x64;	//eco,display,beep,ontimerenable, offtimerenable,power,0,0
	//dataTX[8] = 0x08;	//mute,0,turbo,health, mode(4) mode 01 heat, 02 dry, 03 cool, 07 fan, 08 auto, health(+16), 41=turbo-heat 43=turbo-cool (turbo = 0x40+ 0x01..0x08)
	//dataTX[9] = 0x0f;	//0 -31 ;    15 - 16 0,0,0,0, temp(4) settemp 31 - x
	//dataTX[10] = 0x00;	//0,timerindicator,swingv(3),fan(3) fan+swing modes //0=auto 1=low 2=med 3=high
	//dataTX[11] = 0x00;	//0,offtimer(6),0
	dataTX[12] = 0x00;	//fahrenheit,ontimer(6),0 cf 80=f 0=c
	dataTX[13] = 0x01;	//??
	dataTX[14] = 0x00;	//0,0,halfdegree,0,0,0,0,0
	dataTX[15] = 0x00;	//??
	dataTX[16] = 0x00;	//??
	dataTX[17] = 0x00;	//??
	dataTX[18] = 0x00;	//??
	//dataTX[19] = 0x00;	//sleep on = 1 off=0
	dataTX[20] = 0x00;	//??
	dataTX[21] = 0x00;	//??
	dataTX[22] = 0x00;	//??
	dataTX[23] = 0x00;	//??
	dataTX[24] = 0x00;	//??
	dataTX[25] = 0x00;	//??
	dataTX[26] = 0x00;	//??
	dataTX[27] = 0x00;	//??
	dataTX[28] = 0x00;	//??
	dataTX[30] = 0x00;	//??
	dataTX[31] = 0x00;	//??
	//dataTX[32] = 0x00;	//0,0,0,режим вертикального качания(2),режим вертикальной фиксации(3)
	//dataTX[33] = 0x00;	//0,0,режим горизонтального качания(3),режим горизонтальной фиксации(3)
	dataTX[34] = 0x00;	//??
	dataTX[35] = 0x00;	//??
	dataTX[36] = 0x00;	//??
	dataTX[37] = 0xFF;	//Контрольная сумма
	dataTX[37] = tclacClimate::getChecksum(dataTX, sizeof(dataTX));

	if (switch_climate_mode == climate::CLIMATE_MODE_HEAT) {
		auto heat_tx_raw = tclacClimate::getHex(dataTX, sizeof(dataTX));
		ESP_LOGI("TCL-HEAT",
		         "TX HEAT SET: mode_byte=0x%02X (SET mode low nibble=0x%02X) power_field=0x%02X target_code=0x%02X frame=%s",
		         dataTX[8], static_cast<unsigned>(dataTX[8] & 0x0F), dataTX[7], dataTX[9],
		         heat_tx_raw.c_str());
		this->heat_diag_pending_ = true;
		this->heat_diag_tx_ms_ = millis();
	}

	tclacClimate::sendData(dataTX, sizeof(dataTX));
	allow_take_control = false;
	is_call_control = false;
}

// A5 ACK-only takeover test. This intentionally implements only framing,
// CRC validation and the stock-dongle ACK shape. It does not send commands,
// clock replies, heartbeats, or any legacy BB traffic.
uint16_t tclacClimate::a5_crc16_(const uint8_t *data, size_t len) {
	uint16_t c = 0x0000;
	for (size_t k = 0; k < len; k++) {
		if (k == 8 || k == 9)
			continue;
		c ^= static_cast<uint16_t>(data[k]) << 8;
		for (int i = 0; i < 8; i++)
			c = (c & 0x8000) ? static_cast<uint16_t>((c << 1) ^ 0x1021) : static_cast<uint16_t>(c << 1);
	}
	return c;
}

void tclacClimate::a5_ack_loop_() {
	const uint32_t now = millis();

	// Runtime banner is deliberately emitted from loop(), not setup().  This
	// proves that the custom component is actually executing after the logger
	// and API are fully online, even on builds where early setup logs are lost.
	if (!this->a5_runtime_banner_logged_) {
		this->a5_runtime_banner_logged_ = true;
		this->a5_last_heartbeat_ms_ = now;
		this->a5_last_rx_report_ms_ = now;
		ESP_LOGW("A5-TEST", "RUNTIME START: A5 Phase 3 active; ACK + clock + RSSI + state decoder + test commands; UART=115200 8N1; TX=GPIO3 RX=GPIO4; legacy BB disabled");
		ESP_LOGW("A5-TEST", "A5 MEDIATOR V5: AC owner GPIO3/4; Factory Dongle GPIO6(TX)/GPIO5(RX); AC stream mirrored; Dongle startup 000001 + state query 0B0B + control 0A0A forwarded; type23 suppressed");
	}

	// Warm rejoin/state sync. The Factory Dongle capture proved that its
	// startup INIT frame (payload 00 00 01) is accepted by the TAC-PRO12PEC
	// and is followed by AC replies/state reports. Send it once after every
	// ESP32 boot, even if the AC itself never lost mains power. If no 0C0C
	// state report follows, retry conservatively up to three times.
	if (!this->a5_rejoin_started_ && now >= 750U) {
		this->a5_rejoin_started_ = true;
		this->a5_send_rejoin_init_();
		this->a5_rejoin_attempts_ = 1;
		this->a5_rejoin_next_ms_ = now + 2000U;
	} else if (this->a5_rejoin_started_ && !this->a5_rejoin_state_seen_ &&
			this->a5_rejoin_attempts_ < 3 && static_cast<int32_t>(now - this->a5_rejoin_next_ms_) >= 0) {
		this->a5_send_rejoin_init_();
		this->a5_rejoin_attempts_++;
		this->a5_rejoin_next_ms_ = now + 2000U;
	}

	// Once communication has rejoined, issue one Factory-style 0B0B FFFF
	// parameter query.  Keep this separate from INIT retry ownership: receiving
	// the first 0C0C still stops INIT retries, then the query asks the AC to
	// publish its full parameter set (including persistent settings such as Beep).
	if (this->a5_rejoin_query_pending_ && !this->a5_rejoin_query_sent_ &&
			static_cast<int32_t>(now - this->a5_rejoin_query_due_ms_) >= 0) {
		this->a5_rejoin_query_pending_ = false;
		this->a5_rejoin_query_sent_ = true;
		this->a5_send_rejoin_state_query_();
	}

	if ((now - this->a5_last_heartbeat_ms_) >= 5000U) {
		this->a5_last_heartbeat_ms_ = now;
		ESP_LOGW("A5-TEST", "HEARTBEAT: raw_rx=%lu valid=%lu bad_crc=%lu ack_tx=%lu partial=%u expected=%u",
			static_cast<unsigned long>(this->a5_raw_bytes_),
			static_cast<unsigned long>(this->a5_valid_frames_),
			static_cast<unsigned long>(this->a5_bad_crc_),
			static_cast<unsigned long>(this->a5_acks_sent_),
			static_cast<unsigned>(this->a5_frame_pos_),
			static_cast<unsigned>(this->a5_expected_len_));
		if (this->dongle_uart_ != nullptr) {
			ESP_LOGW("A5-BRIDGE", "DONGLE: raw=%lu valid=%lu bad_crc=%lu cmd_fwd=%lu suppressed=%lu partial=%u expected=%u",
				(unsigned long)this->a5_dongle_raw_bytes_, (unsigned long)this->a5_dongle_valid_frames_,
				(unsigned long)this->a5_dongle_bad_crc_, (unsigned long)this->a5_dongle_commands_forwarded_,
				(unsigned long)this->a5_dongle_frames_suppressed_, (unsigned)this->a5_dongle_frame_pos_,
				(unsigned)this->a5_dongle_expected_len_);
		}
	}

	// Stock module emits an RSSI heartbeat about once a minute.  Delay the first
	// one until the node has been alive for 60s so cold-boot traffic stays clean.
	if (now >= 60000U && (this->a5_last_rssi_ms_ == 0 || (now - this->a5_last_rssi_ms_) >= 60000U)) {
		this->a5_last_rssi_ms_ = now;
		this->a5_send_rssi_();
	}

	if (this->a5_clock_pending_ && static_cast<int32_t>(now - this->a5_ack_due_ms_) >= 0)
		this->a5_send_clock_reply_();
	else if (this->a5_ack_pending_ && static_cast<int32_t>(now - this->a5_ack_due_ms_) >= 0)
		this->a5_send_ack_();

	if (this->a5_frame_pos_ > 0 && (now - this->a5_last_byte_ms_) > 100U) {
		ESP_LOGW("TCL-A5", "RX timeout; discard %u bytes", static_cast<unsigned>(this->a5_frame_pos_));
		this->a5_frame_pos_ = 0;
		this->a5_expected_len_ = 0;
	}

	// Factory Dongle side is a separate UART. Never electrically share its TX
	// with the AC-side TX. Parse complete A5 frames so the ESP32 can arbitrate.
	if (this->dongle_uart_ != nullptr) {
		if (this->a5_dongle_frame_pos_ > 0 && (now - this->a5_dongle_last_byte_ms_) > 100U) {
			ESP_LOGW("A5-BRIDGE", "Dongle RX timeout; discard %u bytes", (unsigned)this->a5_dongle_frame_pos_);
			this->a5_dongle_frame_pos_ = 0;
			this->a5_dongle_expected_len_ = 0;
		}
		while (this->dongle_uart_->available() > 0) {
			uint8_t dv;
			if (!this->dongle_uart_->read_byte(&dv)) break;
			this->a5_dongle_raw_bytes_++;
			this->a5_dongle_parse_byte_(dv);
		}
	}

	uint32_t bytes_this_pass = 0;
	while (esphome::uart::UARTDevice::available() > 0) {
		uint8_t value;
		if (!esphome::uart::UARTDevice::read_byte(&value))
			break;
		this->a5_raw_bytes_++;
		bytes_this_pass++;
		// The factory dongle sees exactly the AC-originated stream. ESP32's own
		// ACK/clock/RSSI frames are NOT mirrored, preventing a synthetic echo.
		if (this->dongle_uart_ != nullptr) this->dongle_uart_->write_byte(value);
		this->a5_parse_byte_(value);
	}
	if (bytes_this_pass > 0 && (now - this->a5_last_rx_report_ms_) >= 1000U) {
		this->a5_last_rx_report_ms_ = now;
		ESP_LOGW("A5-TEST", "RX ACTIVITY: total_raw_bytes=%lu (+%lu this pass)",
			static_cast<unsigned long>(this->a5_raw_bytes_), static_cast<unsigned long>(bytes_this_pass));
	}
}

void tclacClimate::a5_dongle_parse_byte_(uint8_t value) {
	if (this->a5_dongle_frame_pos_ == 0 && value != 0xA5) return;
	if (this->a5_dongle_frame_pos_ >= sizeof(this->a5_dongle_frame_)) {
		this->a5_dongle_frame_pos_ = 0; this->a5_dongle_expected_len_ = 0; return;
	}
	this->a5_dongle_frame_[this->a5_dongle_frame_pos_++] = value;
	this->a5_dongle_last_byte_ms_ = millis();
	if (this->a5_dongle_frame_pos_ == 8) {
		this->a5_dongle_expected_len_ = this->a5_dongle_frame_[7];
		if (this->a5_dongle_expected_len_ < 12 || this->a5_dongle_expected_len_ > sizeof(this->a5_dongle_frame_)) {
			ESP_LOGW("A5-BRIDGE", "Dongle invalid A5 length=%u; resync", (unsigned)this->a5_dongle_expected_len_);
			this->a5_dongle_frame_pos_ = 0; this->a5_dongle_expected_len_ = 0; return;
		}
	}
	if (this->a5_dongle_expected_len_ && this->a5_dongle_frame_pos_ == this->a5_dongle_expected_len_) {
		this->a5_dongle_process_frame_();
		this->a5_dongle_frame_pos_ = 0; this->a5_dongle_expected_len_ = 0;
	}
}

void tclacClimate::a5_dongle_process_frame_() {
	const size_t len = this->a5_dongle_frame_pos_;
	const uint16_t got = (uint16_t(this->a5_dongle_frame_[8]) << 8) | this->a5_dongle_frame_[9];
	const uint16_t calc = this->a5_crc16_(this->a5_dongle_frame_, len);
	if (got != calc) {
		this->a5_dongle_bad_crc_++;
		ESP_LOGW("A5-BRIDGE", "DONGLE CRC FAIL len=%u got=%04X calc=%04X", (unsigned)len, got, calc);
		return;
	}
	this->a5_dongle_valid_frames_++;
	// Optional reverse-engineering logger. Routing/arbitration behavior is unchanged.
	if (this->a5_raw_logger_enabled_) {
		std::string hex;
		char tmp[4];
		for (size_t i = 0; i < len; i++) {
			snprintf(tmp, sizeof(tmp), "%02X", this->a5_dongle_frame_[i]);
			if (!hex.empty()) hex += " ";
			hex += tmp;
		}
		ESP_LOGW("A5-DONGLE-HEX", "RX DONGLE #%lu len=%u frame=%s",
			(unsigned long)this->a5_dongle_valid_frames_, (unsigned)len, hex.c_str());
	}
	const uint8_t type = this->a5_dongle_frame_[3];
	const uint8_t ctr  = this->a5_dongle_frame_[4];
	const uint8_t p0 = len > 10 ? this->a5_dongle_frame_[10] : 0xFF;
	const uint8_t p1 = len > 11 ? this->a5_dongle_frame_[11] : 0xFF;

	// A5 Mediator V5 routing.  Evidence-backed classes only:
	//   21 / 00 00 01 : Factory/TuyaAC startup-init frame.
	//   21 / 0B 0B .. : parameter/state request family (FFFF observed externally).
	//   21 / 0A 0A .. : normal control command.
	// AC-originated bytes are already mirrored byte-for-byte to the Factory Dongle
	// in a5_ack_loop_(), so any real AC response to these forwarded transactions
	// returns to the requester without synthesizing a Dongle-side response here.
	// Dongle type 23 remains suppressed toward AC to prevent duplicate ACK/service
	// ownership while ESP32 remains the permanent AC-side A5 owner.
	const bool init_000001 = (type == 0x21 && len >= 13 && p0 == 0x00 && p1 == 0x00 && this->a5_dongle_frame_[12] == 0x01);
	const bool state_query = (type == 0x21 && p0 == 0x0B && p1 == 0x0B);
	const bool control_cmd = (type == 0x21 && p0 == 0x0A && p1 == 0x0A);
	if (init_000001 || state_query || control_cmd) {
		this->esphome::uart::UARTDevice::write_array(this->a5_dongle_frame_, len);
		this->a5_dongle_commands_forwarded_++;
		const char *kind = init_000001 ? "INIT 000001" : (state_query ? "STATE-QUERY 0B0B" : "CONTROL 0A0A");
		ESP_LOGW("A5-MEDIATOR", "DONGLE -> AC #%lu %s len=%u ctr=%02X",
			(unsigned long)this->a5_dongle_commands_forwarded_, kind, (unsigned)len, ctr);
		return;
	}
	this->a5_dongle_frames_suppressed_++;
	if (type == 0x23) {
		ESP_LOGD("A5-MEDIATOR", "DONGLE type23 suppressed -> AC: ctr=%02X payload=%02X%02X len=%u", ctr, p0, p1, (unsigned)len);
	} else {
		ESP_LOGW("A5-MEDIATOR", "DONGLE UNKNOWN suppressed: type=%02X ctr=%02X payload=%02X%02X len=%u", type, ctr, p0, p1, (unsigned)len);
	}
}

void tclacClimate::a5_parse_byte_(uint8_t value) {
	if (this->a5_frame_pos_ == 0 && value != 0xA5)
		return;
	if (this->a5_frame_pos_ >= sizeof(this->a5_frame_)) {
		ESP_LOGW("TCL-A5", "RX overflow; resync");
		this->a5_frame_pos_ = 0;
		this->a5_expected_len_ = 0;
		return;
	}
	this->a5_frame_[this->a5_frame_pos_++] = value;
	this->a5_last_byte_ms_ = millis();
	if (this->a5_frame_pos_ == 8) {
		this->a5_expected_len_ = this->a5_frame_[7];
		if (this->a5_expected_len_ < 12 || this->a5_expected_len_ > sizeof(this->a5_frame_)) {
			ESP_LOGW("TCL-A5", "Invalid A5 length=%u; resync", static_cast<unsigned>(this->a5_expected_len_));
			this->a5_frame_pos_ = 0;
			this->a5_expected_len_ = 0;
			return;
		}
	}
	if (this->a5_expected_len_ > 0 && this->a5_frame_pos_ == this->a5_expected_len_) {
		this->a5_process_frame_();
		this->a5_frame_pos_ = 0;
		this->a5_expected_len_ = 0;
	}
}

void tclacClimate::a5_process_frame_() {
	const size_t len = this->a5_frame_pos_;
	const uint16_t got = (static_cast<uint16_t>(this->a5_frame_[8]) << 8) | this->a5_frame_[9];
	const uint16_t calc = this->a5_crc16_(this->a5_frame_, len);
	if (got != calc) {
		this->a5_bad_crc_++;
		ESP_LOGW("TCL-A5", "CRC FAIL len=%u got=%04X calc=%04X bad=%lu", static_cast<unsigned>(len), got, calc, static_cast<unsigned long>(this->a5_bad_crc_));
		return;
	}
	this->a5_valid_frames_++;
	const uint8_t link = this->a5_frame_[2];
	const uint8_t type = this->a5_frame_[3];
	const uint8_t ctr = this->a5_frame_[4];
	const uint8_t p0 = len > 10 ? this->a5_frame_[10] : 0xFF;
	const uint8_t p1 = len > 11 ? this->a5_frame_[11] : 0xFF;
	ESP_LOGW("A5-TEST", "RX VALID #%lu len=%u link=%02X type=%02X ctr=%02X payload=%02X%02X crc=%04X", static_cast<unsigned long>(this->a5_valid_frames_), static_cast<unsigned>(len), link, type, ctr, p0, p1, got);

	// AC reports (0x21) require a module response ~50 ms later.  A 10/10
	// clock request is special: the stock module answers with the 17-byte clock
	// reply rather than the plain 12-byte ACK.
	if (type == 0x21 && len >= 12) {
		if (p0 == 0x10 && p1 == 0x10) {
			this->a5_ack_pending_ = false;
			this->a5_clock_pending_ = true;
			this->a5_ack_due_ms_ = millis() + 50U;
			this->a5_ack_ptype_ = 0x10;
			ESP_LOGD("TCL-A5", "CLOCK reply scheduled +50ms");
		} else {
			this->a5_ack_link_ = link;
			this->a5_ack_counter_ = ctr;
			this->a5_ack_ptype_ = p1;
			this->a5_ack_due_ms_ = millis() + 50U;
			this->a5_ack_pending_ = true;
			ESP_LOGD("TCL-A5", "ACK scheduled +50ms: link=%02X ctr=%02X ptype=%02X", link, ctr, p1);
		}
	}

	// Decode AC state/delta reports before scheduling/processing anything else.
	if (type == 0x21 && p0 == 0x0C && p1 == 0x0C) {
		if (this->a5_rejoin_started_ && !this->a5_rejoin_state_seen_) {
			this->a5_rejoin_state_seen_ = true;
			this->a5_rejoin_query_pending_ = true;
			this->a5_rejoin_query_due_ms_ = millis() + 250U;
			ESP_LOGW("A5-REJOIN", "State report received after rejoin INIT; retries stopped (attempt=%u); full-state query scheduled +250ms",
				static_cast<unsigned>(this->a5_rejoin_attempts_));
		}
		if (this->a5_raw_logger_enabled_) {
			char raw[3 * 128 + 1]; size_t pos = 0;
			for (size_t k = 0; k < len && pos + 4 < sizeof(raw); k++)
				pos += snprintf(raw + pos, sizeof(raw) - pos, "%02X%s", this->a5_frame_[k], (k + 1 < len) ? " " : "");
			ESP_LOGW("A5-STATE-RAW", "len=%u frame=%s", static_cast<unsigned>(len), raw);
		}
		this->a5_decode_state_();
	}

	// Command replies use type 0x23 and xx/0A. 0x80 is the normal ACK seen
	// for the first commands; keep other status bytes visible rather than guessing.
	if (type == 0x23 && p1 == 0x0A) {
		if (p0 == 0x80)
			ESP_LOGW("A5-PHASE2", "COMMAND ACK received from AC: status=%02X echoed_ctr=%02X", p0, this->a5_frame_[5]);
		else
			ESP_LOGW("A5-PHASE4", "COMMAND RESPONSE from AC: status=%02X echoed_ctr=%02X (meaning not assumed)", p0, this->a5_frame_[5]);
	}
}

void tclacClimate::a5_send_ack_() {
	uint8_t f[12] = {0xA5, 0x01, this->a5_ack_link_, 0x23, 0x00, this->a5_ack_counter_, 0x00, 0x0C, 0x00, 0x00, 0x80, this->a5_ack_ptype_};
	const uint16_t crc = this->a5_crc16_(f, sizeof(f));
	f[8] = static_cast<uint8_t>(crc >> 8);
	f[9] = static_cast<uint8_t>(crc & 0xFF);
	this->esphome::uart::UARTDevice::write_array(f, sizeof(f));
	this->a5_ack_pending_ = false;
	this->a5_acks_sent_++;
	ESP_LOGW("A5-TEST", "TX ACK #%lu: ctr=%02X ptype=%02X crc=%04X frame=A5 01 %02X 23 00 %02X 00 0C %02X %02X 80 %02X", static_cast<unsigned long>(this->a5_acks_sent_), this->a5_ack_counter_, this->a5_ack_ptype_, crc, this->a5_ack_link_, this->a5_ack_counter_, f[8], f[9], this->a5_ack_ptype_);
}


void tclacClimate::a5_send_frame_(uint8_t *f, size_t len, const char *label) {
	f[7] = static_cast<uint8_t>(len);
	f[8] = 0; f[9] = 0;
	const uint16_t crc = this->a5_crc16_(f, len);
	f[8] = static_cast<uint8_t>(crc >> 8);
	f[9] = static_cast<uint8_t>(crc & 0xFF);
	this->esphome::uart::UARTDevice::write_array(f, len);
	ESP_LOGW("A5-PHASE2", "TX %s len=%u crc=%04X", label, static_cast<unsigned>(len), crc);
}

void tclacClimate::a5_send_clock_reply_() {
	// Shape copied directly from the stock-module capture. Unix time is big-endian.
	uint32_t unix_time = static_cast<uint32_t>(::time(nullptr));
	uint8_t f[17] = {0xA5,0x01,0x00,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x10,
		static_cast<uint8_t>(unix_time >> 24), static_cast<uint8_t>(unix_time >> 16),
		static_cast<uint8_t>(unix_time >> 8), static_cast<uint8_t>(unix_time), 0xFB};
	this->a5_send_frame_(f, sizeof(f), "CLOCK REPLY");
	this->a5_clock_pending_ = false;
	this->a5_clock_replies_sent_++;
	ESP_LOGW("A5-PHASE2", "CLOCK reply #%lu unix=%lu", static_cast<unsigned long>(this->a5_clock_replies_sent_), static_cast<unsigned long>(unix_time));
}

void tclacClimate::a5_send_rejoin_init_() {
	// Exact startup-init shape captured from the Factory Dongle:
	// A5 01 00 21 03 00 00 0D 08 05 00 00 01
	// CRC is regenerated by a5_send_frame_() rather than hard-coded.
	uint8_t f[13] = {0xA5,0x01,0x00,0x21,0x03,0x00,0x00,0x0D,0x00,0x00,0x00,0x00,0x01};
	this->a5_send_frame_(f, sizeof(f), "REJOIN INIT 000001");
	ESP_LOGW("A5-REJOIN", "Factory-style rejoin INIT sent (next attempt %u/3 if no 0C0C state)",
		static_cast<unsigned>(this->a5_rejoin_attempts_ + 1));
}

void tclacClimate::a5_send_rejoin_state_query_() {
	// Factory-Dongle/OpenBeken startup capture:
	// A5 01 01 21 04 00 00 0E .. .. 0B 0B FF FF
	// Ask for the parameter/state set once after a successful warm rejoin.
	uint8_t f[14] = {0xA5,0x01,0x01,0x21,0x04,0x00,0x00,0x0E,0x00,0x00,0x0B,0x0B,0xFF,0xFF};
	this->a5_send_frame_(f, sizeof(f), "REJOIN STATE-QUERY 0B0B FFFF");
	ESP_LOGW("A5-REJOIN", "Factory-style full-state query 0B0B FFFF sent once after rejoin");
}

void tclacClimate::a5_send_rssi_() {
	// Conservative fixed lab value for protocol compatibility.  This is only a
	// module heartbeat; it does not affect climate control.
	const int32_t dbm = -50;
	uint8_t f[18] = {0xA5,0x01,0x00,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x0C,0x02,0x64,
		static_cast<uint8_t>(dbm >> 24), static_cast<uint8_t>(dbm >> 16), static_cast<uint8_t>(dbm >> 8), static_cast<uint8_t>(dbm)};
	this->a5_send_frame_(f, sizeof(f), "RSSI HEARTBEAT");
	this->a5_rssi_sent_++;
}

void tclacClimate::a5_log_state_() {
	static const char *const MODES[5] = {"AUTO", "COOL", "DRY", "FAN", "HEAT"};
	char power[12], mode[16], sp[16], room[16], out[16], fan[12], ct[16], ca[16], pw[16];
	if (a5_state_power_valid_) snprintf(power,sizeof(power),"%s",a5_state_power_?"ON":"OFF"); else snprintf(power,sizeof(power),"?");
	if (a5_state_mode_valid_ && a5_state_mode_ <= 4) snprintf(mode,sizeof(mode),"%s",MODES[a5_state_mode_]); else snprintf(mode,sizeof(mode),"?");
	if (a5_state_setpoint_valid_) snprintf(sp,sizeof(sp),"%.1fC",a5_state_setpoint_c_); else snprintf(sp,sizeof(sp),"?");
	if (a5_state_room_valid_) snprintf(room,sizeof(room),"%.1fC",a5_state_room_c_); else snprintf(room,sizeof(room),"?");
	if (a5_state_outdoor_valid_) snprintf(out,sizeof(out),"%.1fC",a5_state_outdoor_c_); else snprintf(out,sizeof(out),"?");
	if (a5_state_fan_valid_) snprintf(fan,sizeof(fan),"%u",a5_state_fan_); else snprintf(fan,sizeof(fan),"?");
	if (a5_state_comp_target_valid_) snprintf(ct,sizeof(ct),"%u%%",a5_state_comp_target_); else snprintf(ct,sizeof(ct),"?");
	if (a5_state_comp_actual_valid_) snprintf(ca,sizeof(ca),"%u%%",a5_state_comp_actual_); else snprintf(ca,sizeof(ca),"?");
	if (a5_state_power_w_valid_) snprintf(pw,sizeof(pw),"%uW",a5_state_power_w_); else snprintf(pw,sizeof(pw),"?");
	ESP_LOGW("A5-STATE", "Power=%s Mode=%s Setpoint=%s Room=%s Outdoor=%s Fan=%s CompTarget=%s CompActual=%s InputPower=%s",
		power,mode,sp,room,out,fan,ct,ca,pw);
}

void tclacClimate::a5_decode_state_() {
	const size_t n=this->a5_frame_pos_;
	if (n < 13 || a5_frame_[10]!=0x0C || a5_frame_[11]!=0x0C) return;
	// Repo report records begin at byte 13. Known telemetry fields 02/03/5C/60/
	// 64/65/72/C0 are wide and carry the useful 16-bit value in bytes +3,+4.
	// Narrow records carry their value at +1. Reports are deltas, so cache values.
	for (size_t i=13; i+1<n; ) {
		uint8_t fid=a5_frame_[i];
		bool known_wide=(fid==0x02||fid==0x03||fid==0x5C||fid==0x60||fid==0x64||fid==0x65||fid==0x72||fid==0xC0||fid==0x95||fid==0xBD||fid==0xBE||fid==0xBF);
		if (known_wide && i+4<n && a5_frame_[i+1]==0x00 && a5_frame_[i+2]==0x00) {
			uint16_t v=(uint16_t(a5_frame_[i+3])<<8)|a5_frame_[i+4];
			switch(fid) {
				case 0x02: if(v>=1600&&v<=3100){a5_state_setpoint_c_=v/100.0f;a5_state_setpoint_valid_=true;} break;
				case 0x03: if(v>=500&&v<=5000){a5_state_room_c_=v/100.0f;a5_state_room_valid_=true;} break;
				case 0x5C: if(v<=6500){a5_state_coil_c_=v/100.0f;a5_state_coil_valid_=true;} break;
				case 0x60: if(v>=500&&v<=6500){a5_state_outdoor_c_=v/100.0f;a5_state_outdoor_valid_=true;} break;
				case 0x64: if(v<=5000){a5_state_power_w_=v;a5_state_power_w_valid_=true;} break;
				case 0x65: if(v<=100){a5_state_comp_actual_=v;a5_state_comp_actual_valid_=true;} break;
				case 0xC0: if(v<=200){a5_state_comp_target_=v;a5_state_comp_target_valid_=true;} break;
				default: break;
			}
			i += 6; continue;
		}
		// Length-prefixed fields from the repo; skip them without losing alignment.
		if ((fid==0x38||fid==0x39) && i+1<n) {
			size_t l=a5_frame_[i+1]; if(i+2+l<=n){ i += l+3; continue; }
		}
		uint8_t v=a5_frame_[i+1];
		switch(fid) {
			case 0x01: a5_state_power_=(v!=0); a5_state_power_valid_=true; break;
			case 0x12: if(v<=4){a5_state_mode_=v;a5_state_mode_valid_=true;} break;
			case 0x05: if(v<=15){a5_state_fan_=v;a5_state_fan_valid_=true;} break;
			case 0x11: a5_state_vertical_louver_=v; a5_state_vertical_louver_valid_=true; break;
			case 0x0E: a5_state_horizontal_louver_=v; a5_state_horizontal_louver_valid_=true; break;
			case 0x13: a5_state_eco_=(v!=0); a5_state_eco_valid_=true; break;
			case 0x22: if(v<=3){a5_state_sleep_=v; a5_state_sleep_valid_=true;} break;
			case 0x1E: a5_state_display_light_=(v!=0); a5_state_display_light_valid_=true; break;
			case 0x25: a5_state_beep_=(v!=0); a5_state_beep_valid_=true; break;
			case 0x15: a5_state_health_=(v!=0); a5_state_health_valid_=true; break;
			case 0x27: a5_state_drying_=(v!=0); a5_state_drying_valid_=true; break;
			default: break;
		}
		i += 3;
	}
	this->a5_publish_state_();
	this->a5_log_state_();
}

void tclacClimate::a5_send_test_setpoint_(uint16_t centi_c, uint8_t degf) {
	if (!this->a5_ack_only_) { ESP_LOGW("A5-PHASE3", "Test command refused: A5 takeover mode is not active"); return; }
	uint8_t ctr=this->a5_command_counter_++;
	uint8_t f[24] = {0xA5,0x01,0x01,0x21,ctr,0x00,0x00,0x00,0x00,0x00,
		0x0A,0x0A, 0x00,0x02,0x00,0x00,(uint8_t)(centi_c>>8),(uint8_t)centi_c,
		0x02,0x27,0x00,0x00,0x00,degf};
	char label[48]; snprintf(label,sizeof(label),"COMMAND setpoint=%.1fC/%uF",centi_c/100.0f,degf);
	this->a5_send_frame_(f,sizeof(f),label); this->a5_commands_sent_++;
	ESP_LOGW("A5-PHASE3", "TEST COMMAND #%lu sent: setpoint=%.1fC display=%uF counter=%02X",
		(unsigned long)a5_commands_sent_,centi_c/100.0f,degf,ctr);
}
void tclacClimate::a5_send_test_setpoint_24c(){ this->a5_send_test_setpoint_(2400,75); }
void tclacClimate::a5_send_test_setpoint_25c(){ this->a5_send_test_setpoint_(2500,77); }
void tclacClimate::a5_send_test_setpoint_26c(){ this->a5_send_test_setpoint_(2600,79); }

void tclacClimate::a5_send_test_power_(bool on) {
	if (!this->a5_ack_only_) { ESP_LOGW("A5-PHASE4", "Power command refused: A5 takeover mode is not active"); return; }
	const uint8_t ctr = this->a5_command_counter_++;
	// Repo Command::field(0x01,value): <00> <01> <00|01>.
	uint8_t f[15] = {0xA5,0x01,0x01,0x21,ctr,0x00,0x00,0x00,0x00,0x00,
		0x0A,0x0A,0x00,0x01,static_cast<uint8_t>(on ? 0x01 : 0x00)};
	this->a5_send_frame_(f, sizeof(f), on ? "COMMAND power=ON" : "COMMAND power=OFF");
	this->a5_commands_sent_++;
	ESP_LOGW("A5-PHASE4", "TEST POWER #%lu sent: %s counter=%02X",
		static_cast<unsigned long>(this->a5_commands_sent_), on ? "ON" : "OFF", ctr);
}

void tclacClimate::a5_send_test_power_on(){ this->a5_send_test_power_(true); }
void tclacClimate::a5_send_test_power_off(){ this->a5_send_test_power_(false); }


void tclacClimate::a5_send_setpoint_c_(float celsius) {
	if (!this->a5_ack_only_) return;

	// Enforce the configured grid at the command boundary as well as in the
	// advertised Climate traits. This prevents API/service calls from bypassing
	// the Home Assistant UI step. Supported grids are 1.0 C and 0.5 C.
	const float step = (this->target_temperature_step_ < 0.75f) ? 0.5f : 1.0f;
	float normalized = roundf(celsius / step) * step;
	if (normalized < 16.0f) normalized = 16.0f;
	if (normalized > 31.0f) normalized = 31.0f;

	const int centi = (int) lroundf(normalized * 100.0f);
	// A5 field 0x27 follows the appliance's display ladder used by the proven
	// command format: 27.0 C=81, with each 0.5 C step changing this byte by 1.
	int degf = 81 + (centi - 2700) / 50;
	if (degf < 59) degf = 59;
	if (degf > 89) degf = 89;

	if (fabsf(normalized - celsius) > 0.01f) {
		ESP_LOGW("A5-SETPOINT", "Normalized requested %.2fC -> %.1fC (step %.1fC)",
		         celsius, normalized, step);
	}
	this->a5_send_test_setpoint_((uint16_t) centi, (uint8_t) degf);
}

void tclacClimate::a5_send_mode_(uint8_t raw) {
	if (!this->a5_ack_only_ || raw>4) return;
	uint8_t ctr=this->a5_command_counter_++;
	uint8_t f[15]={0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,0x00,0x12,raw};
	this->a5_send_frame_(f,sizeof(f),"CLIMATE mode"); this->a5_commands_sent_++;
}

void tclacClimate::a5_send_fan_(uint8_t speed) {
	if (!this->a5_ack_only_) return;
	if(speed>7) speed=7;
	uint8_t ctr=this->a5_command_counter_++;
	uint8_t autoflag=(speed==0)?1:0;
	uint8_t f[18]={0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,
		0x00,0x73,autoflag,0x00,0x05,speed};
	this->a5_send_frame_(f,sizeof(f),"CLIMATE fan"); this->a5_commands_sent_++;
}

void tclacClimate::a5_send_swing_(climate::ClimateSwingMode mode) {
	if (!this->a5_ack_only_) return;
	// A5 louver fields: 0x11 vertical, 0x0E horizontal. 0x01 is the
	// full-axis sweep and 0x08 is parked/swing-off, as documented upstream.
	uint8_t vertical = 0x08;
	uint8_t horizontal = 0x08;
	switch (mode) {
		case climate::CLIMATE_SWING_VERTICAL: vertical = 0x01; break;
		case climate::CLIMATE_SWING_HORIZONTAL: horizontal = 0x01; break;
		case climate::CLIMATE_SWING_BOTH: vertical = 0x01; horizontal = 0x01; break;
		case climate::CLIMATE_SWING_OFF: default: break;
	}
	uint8_t ctr=this->a5_command_counter_++;
	uint8_t f[18]={0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,
		0x00,0x11,vertical,0x00,0x0E,horizontal};
	this->a5_send_frame_(f,sizeof(f),"CLIMATE swing"); this->a5_commands_sent_++;
}

void tclacClimate::a5_send_preset_(climate::ClimatePreset value) {
	if (!this->a5_ack_only_) return;
	uint8_t eco=0, sleep=0;
	if (value == ClimatePreset::CLIMATE_PRESET_ECO) eco=1;
	else if (value == ClimatePreset::CLIMATE_PRESET_SLEEP) sleep=1;  // Standard sleep
	else if (value != ClimatePreset::CLIMATE_PRESET_NONE) {
		ESP_LOGW("TCL-A5", "Unsupported A5 preset requested"); return;
	}
	uint8_t ctr=this->a5_command_counter_++;
	uint8_t f[18]={0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,
		0x00,0x13,eco,0x00,0x22,sleep};
	this->a5_send_frame_(f,sizeof(f),"CLIMATE preset"); this->a5_commands_sent_++;
}

void tclacClimate::a5_send_bool_field_(uint8_t field, bool enabled, const char *label) {
	if (!this->a5_ack_only_) return;
	const uint8_t ctr = this->a5_command_counter_++;
	uint8_t f[15] = {0xA5,0x01,0x01,0x21,ctr,0,0,0,0,0,0x0A,0x0A,0x00,field,
		static_cast<uint8_t>(enabled ? 0x01 : 0x00)};
	this->a5_send_frame_(f, sizeof(f), label);
	this->a5_commands_sent_++;
}

void tclacClimate::a5_send_beep_setting_(bool enabled) {
	// Factory Dongle capture: 0A 0A 00 25 00/01 is the persistent
	// beeper-policy setting. Keep it as a single-field command so no
	// louver/swing field (0x11/0x0E) can be modified as a side effect.
	this->a5_send_bool_field_(0x25, enabled, enabled ? "BEEP setting=ON" : "BEEP setting=OFF");
}

void tclacClimate::a5_publish_state_() {
	if (a5_state_room_valid_) {
		this->current_temperature=a5_state_room_c_;
		if(this->room_temperature_sensor_!=nullptr) this->room_temperature_sensor_->publish_state(a5_state_room_c_);
	}
	if (a5_state_outdoor_valid_ && this->outdoor_ambient_candidate_sensor_!=nullptr)
		this->outdoor_ambient_candidate_sensor_->publish_state(a5_state_outdoor_c_);
	if (a5_state_coil_valid_ && this->indoor_coil_temperature_sensor_!=nullptr)
		this->indoor_coil_temperature_sensor_->publish_state(a5_state_coil_c_);
	if (a5_state_power_w_valid_ && this->a5_input_power_sensor_!=nullptr)
		this->a5_input_power_sensor_->publish_state(a5_state_power_w_);
	if (a5_state_comp_actual_valid_ && this->a5_compressor_actual_sensor_!=nullptr)
		this->a5_compressor_actual_sensor_->publish_state(a5_state_comp_actual_);
	if (a5_state_comp_target_valid_ && this->a5_compressor_target_sensor_!=nullptr)
		this->a5_compressor_target_sensor_->publish_state(a5_state_comp_target_);
	if (a5_state_setpoint_valid_) this->target_temperature=a5_state_setpoint_c_;
	if (a5_state_power_valid_ && !a5_state_power_) {
		this->mode=climate::CLIMATE_MODE_OFF;
		this->action=climate::CLIMATE_ACTION_OFF;
	} else if (a5_state_mode_valid_) {
		switch(a5_state_mode_) {
			case 0: this->mode=climate::CLIMATE_MODE_AUTO; break;
			case 1: this->mode=climate::CLIMATE_MODE_COOL; break;
			case 2: this->mode=climate::CLIMATE_MODE_DRY; break;
			case 3: this->mode=climate::CLIMATE_MODE_FAN_ONLY; break;
			case 4: this->mode=climate::CLIMATE_MODE_HEAT; break;
		}
		// A5 Action: use Compressor Actual as the physical activity signal.
		// TAC-PRO12PEC captures show Target may lead Actual at startup and drop
		// before Actual at shutdown, so Target is intentionally NOT used here.
		// User-facing activity threshold with hysteresis:
		//   Actual <= 20% : IDLE
		//   Actual >= 25% : active action for COOL/HEAT/DRY
		//   Actual 21..24%: keep the previous action to prevent flicker.
		if (this->mode==climate::CLIMATE_MODE_FAN_ONLY) {
			this->action=climate::CLIMATE_ACTION_FAN;
		} else if (this->mode==climate::CLIMATE_MODE_COOL ||
		           this->mode==climate::CLIMATE_MODE_HEAT ||
		           this->mode==climate::CLIMATE_MODE_DRY) {
			if (a5_state_comp_actual_valid_) {
				const uint16_t actual = a5_state_comp_actual_;
				if (actual <= 20) {
					this->action=climate::CLIMATE_ACTION_IDLE;
				} else if (actual >= 25) {
					if (this->mode==climate::CLIMATE_MODE_HEAT)
						this->action=climate::CLIMATE_ACTION_HEATING;
					else if (this->mode==climate::CLIMATE_MODE_DRY)
						this->action=climate::CLIMATE_ACTION_DRYING;
					else
						this->action=climate::CLIMATE_ACTION_COOLING;
				}
				// 21..24%: deliberately retain the previous Action (hysteresis).
			} else {
				// Until Actual has been reported, do not infer activity from Target.
				this->action=climate::CLIMATE_ACTION_IDLE;
			}
		} else if (this->mode==climate::CLIMATE_MODE_AUTO) {
			// AUTO Action: Compressor Actual decides whether the refrigeration
			// circuit is meaningfully active; Indoor Coil vs Indoor Temperature
			// decides the thermal direction.  A small +/-2 C deadband prevents
			// COOLING/HEATING chatter while the coil is near room temperature.
			//   Actual <= 20% : IDLE
			//   Actual 21..24%: retain previous Action (compressor hysteresis)
			//   Actual >= 25% and Coil <= Room-2C : COOLING
			//   Actual >= 25% and Coil >= Room+2C : HEATING
			//   Within +/-2C: retain previous Action (thermal deadband).
			if (a5_state_comp_actual_valid_) {
				const uint16_t actual = a5_state_comp_actual_;
				if (actual <= 20) {
					this->action=climate::CLIMATE_ACTION_IDLE;
				} else if (actual >= 25 && a5_state_coil_valid_ && a5_state_room_valid_) {
					const float delta = a5_state_coil_c_ - a5_state_room_c_;
					if (delta <= -2.0f)
						this->action=climate::CLIMATE_ACTION_COOLING;
					else if (delta >= 2.0f)
						this->action=climate::CLIMATE_ACTION_HEATING;
					// -2C < delta < +2C: deliberately retain previous Action.
				}
				// Actual 21..24%: deliberately retain previous Action.
			} else {
				// Until Actual has been reported, do not infer activity.
				this->action=climate::CLIMATE_ACTION_IDLE;
			}
		}
	}
	if(a5_state_fan_valid_) {
		switch(a5_state_fan_) {
			case 0: this->fan_mode=climate::CLIMATE_FAN_AUTO; break;
			case 1: this->fan_mode=climate::CLIMATE_FAN_QUIET; break;
			case 2: this->fan_mode=climate::CLIMATE_FAN_LOW; break;
			case 3: this->fan_mode=climate::CLIMATE_FAN_MIDDLE; break;
			case 4: this->fan_mode=climate::CLIMATE_FAN_MEDIUM; break;
			case 5: this->fan_mode=climate::CLIMATE_FAN_HIGH; break;
			case 6: this->fan_mode=climate::CLIMATE_FAN_FOCUS; break;
			case 7: this->fan_mode=climate::CLIMATE_FAN_DIFFUSE; break;
		}
	}
	if (this->swing_feature_enabled_ && a5_state_vertical_louver_valid_ && a5_state_horizontal_louver_valid_) {
		const bool v_sweep = (a5_state_vertical_louver_ & 0x08) == 0;
		const bool h_sweep = (a5_state_horizontal_louver_ & 0x08) == 0;
		if (v_sweep && h_sweep) this->swing_mode = climate::CLIMATE_SWING_BOTH;
		else if (v_sweep) this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
		else if (h_sweep) this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
		else this->swing_mode = climate::CLIMATE_SWING_OFF;
	}
	if (this->preset_feature_enabled_) {
		if (a5_state_sleep_valid_ && a5_state_sleep_ != 0) this->preset = ClimatePreset::CLIMATE_PRESET_SLEEP;
		else if (a5_state_eco_valid_ && a5_state_eco_) this->preset = ClimatePreset::CLIMATE_PRESET_ECO;
		else if (a5_state_sleep_valid_ || a5_state_eco_valid_) this->preset = ClimatePreset::CLIMATE_PRESET_NONE;
	}
	this->publish_state();
}

// Отправка данных в кондиционер
void tclacClimate::sendData(byte * message, byte size) {
	if (this->a5_ack_only_) {
		ESP_LOGW("TCL-A5", "Blocked legacy BB/control TX during A5 ACK-only test");
		return;
	}
	if (this->proxy_mode_ && this->dongle_uart_ != nullptr) {
		this->proxy_queue_or_send_(message, size);
		return;
	}
	tclacClimate::dataShow(1,1);
	this->esphome::uart::UARTDevice::write_array(message, size);
	ESP_LOGD("TCL", "Message to TCL sent");
	tclacClimate::dataShow(1,0);
}

void tclacClimate::proxy_forward_dongle_to_ac_() {
	while (this->dongle_uart_->available() > 0) {
		uint8_t value;
		if (!this->dongle_uart_->read_byte(&value))
			break;

		// Forward immediately. The factory dongle and the AC are electrically
		// isolated from each other by the ESP32, so there is no TX contention.
		this->esphome::uart::UARTDevice::write_byte(value);
		this->proxy_last_activity_ms_ = millis();
		this->proxy_last_dongle_activity_ms_ = this->proxy_last_activity_ms_;
		if (!this->dual_phase_fast_)
			this->proxy_parse_dongle_byte_(value);
	}
}

void tclacClimate::proxy_parse_dongle_byte_(uint8_t value) {
	const uint32_t now = millis();
	if (this->dongle_frame_pos_ > 0 && (now - this->dongle_frame_last_byte_ms_) > 250) {
		this->dongle_frame_pos_ = 0;
		this->dongle_frame_expected_len_ = 0;
	}

	if (this->dongle_frame_pos_ == 0 && value != 0xBB)
		return;
	if (this->dongle_frame_pos_ >= sizeof(this->dongle_frame_)) {
		this->dongle_frame_pos_ = 0;
		this->dongle_frame_expected_len_ = 0;
		return;
	}

	this->dongle_frame_[this->dongle_frame_pos_++] = value;
	this->dongle_frame_last_byte_ms_ = now;
	if (this->dongle_frame_pos_ == 5) {
		this->dongle_frame_expected_len_ = static_cast<size_t>(this->dongle_frame_[4]) + 6U;
		if (this->dongle_frame_expected_len_ < 6U ||
		    this->dongle_frame_expected_len_ > sizeof(this->dongle_frame_)) {
			this->dongle_frame_pos_ = 0;
			this->dongle_frame_expected_len_ = 0;
		}
	}

	if (this->dongle_frame_expected_len_ != 0 &&
	    this->dongle_frame_pos_ == this->dongle_frame_expected_len_) {
		if (this->proxy_log_packets_) {
			auto raw = tclacClimate::getHex(this->dongle_frame_, static_cast<byte>(this->dongle_frame_expected_len_));
			ESP_LOGD("TCL-PROXY", "Dongle -> AC [%u]: %s",
			         static_cast<unsigned>(this->dongle_frame_expected_len_), raw.c_str());
		}
		this->dongle_frame_pos_ = 0;
		this->dongle_frame_expected_len_ = 0;
	}
}

void tclacClimate::proxy_queue_or_send_(const uint8_t *message, size_t size) {
	if (this->dual_phase_fast_) {
		ESP_LOGD("TCL-DUAL", "Local ESPHome TX suppressed during 115200 boot phase");
		return;
	}
	if (size == 0 || size > sizeof(this->pending_tx_)) {
		ESP_LOGW("TCL-PROXY", "Rejected local TX length %u", static_cast<unsigned>(size));
		return;
	}

	const uint32_t now = millis();
	if ((now - this->proxy_last_activity_ms_) >= this->proxy_idle_time_ms_ &&
	    this->dongle_uart_->available() == 0) {
		this->dataShow(1, 1);
		this->esphome::uart::UARTDevice::write_array(message, size);
		this->proxy_last_activity_ms_ = millis();
		if (this->proxy_log_packets_) {
			auto raw = tclacClimate::getHex(const_cast<byte *>(message), static_cast<byte>(size));
			ESP_LOGD("TCL-PROXY", "ESPHome -> AC [%u]: %s", static_cast<unsigned>(size), raw.c_str());
		}
		this->dataShow(1, 0);
		return;
	}

	std::memcpy(this->pending_tx_, message, size);
	this->pending_tx_len_ = size;
	this->pending_tx_valid_ = true;
	ESP_LOGD("TCL-PROXY", "Local command queued until UART is idle");
}

void tclacClimate::proxy_try_send_pending_() {
	if (!this->pending_tx_valid_)
		return;
	const uint32_t now = millis();
	if ((now - this->proxy_last_activity_ms_) < this->proxy_idle_time_ms_)
		return;
	if (this->dongle_uart_->available() > 0)
		return;

	this->dataShow(1, 1);
	this->esphome::uart::UARTDevice::write_array(this->pending_tx_, this->pending_tx_len_);
	this->proxy_last_activity_ms_ = millis();
	if (this->proxy_log_packets_) {
		auto raw = tclacClimate::getHex(this->pending_tx_, static_cast<byte>(this->pending_tx_len_));
		ESP_LOGD("TCL-PROXY", "ESPHome -> AC queued [%u]: %s",
		         static_cast<unsigned>(this->pending_tx_len_), raw.c_str());
	}
	this->dataShow(1, 0);
	this->pending_tx_valid_ = false;
	this->pending_tx_len_ = 0;
}

// Преобразование байта в читабельный формат
String tclacClimate::getHex(byte *message, byte size) {
	String raw;
	raw.reserve(static_cast<unsigned int>(size) * 3U);
	char hex_byte[3];
	for (int i = 0; i < size; i++) {
		if (i > 0)
			raw += ' ';
		std::snprintf(hex_byte, sizeof(hex_byte), "%02X", message[i]);
		raw += hex_byte;
	}
	return raw;
}

// Вычисление контрольной суммы
byte tclacClimate::getChecksum(const byte * message, size_t size) {
	byte position = size - 1;
	byte crc = 0;
	for (int i = 0; i < position; i++)
		crc ^= message[i];
	return crc;
}

// Мигаем светодиодами
void tclacClimate::dataShow(bool flow, bool shine) {
	if (module_display_status_){
		if (flow == 0){
			if (shine == 1){
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_RX_LED
				this->rx_led_pin_->digital_write(false);
#endif
			}
		}
		if (flow == 1) {
			if (shine == 1){
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(true);
#endif
			} else {
#ifdef CONF_TX_LED
				this->tx_led_pin_->digital_write(false);
#endif
			}
		}
	}
}

// Действия с данными из конфига

// Получение состояния пищалки
void tclacClimate::set_beeper_state(bool state) {
	this->beeper_status_ = state;
	// Apply a user change immediately only after a checksum-valid status frame
	// has populated the current climate state. During boot this only stores the
	// requested policy and cannot transmit a partial command.
	if (allow_take_control)
		tclacClimate::takeControl();
}
// Получение состояния дисплея кондиционера
void tclacClimate::set_display_state(bool state) {
	this->display_status_ = state;
	if (allow_take_control)
		tclacClimate::takeControl();
}
// Получение состояния режима принудительного применения настроек
void tclacClimate::set_force_mode_state(bool state) {
	this->force_mode_status_ = state;
}
// Получение пина светодиода приема данных
#ifdef CONF_RX_LED
void tclacClimate::set_rx_led_pin(GPIOPin *rx_led_pin) {
	this->rx_led_pin_ = rx_led_pin;
}
#endif
// Получение пина светодиода передачи данных
#ifdef CONF_TX_LED
void tclacClimate::set_tx_led_pin(GPIOPin *tx_led_pin) {
	this->tx_led_pin_ = tx_led_pin;
}
#endif
// Получение состояния светодиодов связи модуля
void tclacClimate::set_module_display_state(bool state) {
	this->module_display_status_ = state;
}
// Получение режима фиксации вертикальной заслонки
void tclacClimate::set_vertical_airflow(AirflowVerticalDirection direction) {
	this->vertical_direction_ = direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Получение режима фиксации горизонтальных заслонок
void tclacClimate::set_horizontal_airflow(AirflowHorizontalDirection direction) {
	this->horizontal_direction_ = direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Получение режима качания вертикальной заслонки
void tclacClimate::set_vertical_swing_direction(VerticalSwingDirection direction) {
	this->vertical_swing_direction_ = direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Получение доступных режимов работы кондиционера
void tclacClimate::set_supported_modes(climate::ClimateModeMask modes) {
	this->supported_modes_ = modes;
}
// Получение режима качания горизонтальных заслонок
void tclacClimate::set_horizontal_swing_direction(HorizontalSwingDirection direction) {
	horizontal_swing_direction_ = direction;
	if (force_mode_status_){
		if (allow_take_control){
			tclacClimate::takeControl();
		}
	}
}
// Получение доступных скоростей вентилятора
void tclacClimate::set_supported_fan_modes(climate::ClimateFanModeMask modes){
	this->supported_fan_modes_ = modes;
}
// Получение доступных режимов качания заслонок
void tclacClimate::set_supported_swing_modes(climate::ClimateSwingModeMask modes) {
	this->supported_swing_modes_ = modes;
}
// Получение доступных предустановок
void tclacClimate::set_supported_presets(climate::ClimatePresetMask presets) {
  this->supported_presets_ = presets;
}


}
}
