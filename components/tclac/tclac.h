/**
* Created by Miguel Ángel López on 20/07/19
* and modified by xaxexa
* Refactoring & component making:
* Nightingale with a soldering iron 15.03.2024
**/

#ifndef TCL_ESP_TCL_H
#define TCL_ESP_TCL_H

#include "esphome.h"
#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include <cmath>
#include <limits>

namespace esphome {
namespace tclac {

#define SET_TEMP_MASK	0b00001111

#define MODE_POS		7
#define MODE_MASK		0b00111111

#define MODE_AUTO		0b00110101
#define MODE_COOL		0b00110001
#define MODE_DRY		0b00110011
#define MODE_FAN_ONLY		0b00110010
#define MODE_HEAT		0b00110100

#define FAN_SPEED_POS	8
#define FAN_QUIET_POS	33

#define FAN_AUTO		0b10000000	// auto
#define FAN_QUIET		0x80		// silent
#define FAN_LOW			0b10010000	// |
#define FAN_MIDDLE		0b11000000	// ||
#define FAN_MEDIUM		0b10100000	// |||
#define FAN_HIGH		0b11010000	// ||||
#define FAN_FOCUS		0b10110000	// |||||
#define FAN_DIFFUSE		0b10000000	// POWER [7]
#define FAN_SPEED_MASK	0b11110000	// FAN SPEED MASK

#define SWING_POS			10
#define SWING_OFF			0b00000000
#define SWING_HORIZONTAL	0b00100000
#define SWING_VERTICAL		0b01000000
#define SWING_BOTH			0b01100000
#define SWING_MODE_MASK		0b01100000

using climate::ClimateCall;
using climate::ClimateMode;
using climate::ClimatePreset;
using climate::ClimateTraits;
using climate::ClimateFanMode;
using climate::ClimateSwingMode;

enum class VerticalSwingDirection : uint8_t {
	UP_DOWN = 0,
	UPSIDE = 1,
	DOWNSIDE = 2,
};
enum class HorizontalSwingDirection : uint8_t {
	LEFT_RIGHT = 0,
	LEFTSIDE = 1,
	CENTER = 2,
	RIGHTSIDE = 3,
};
enum class AirflowVerticalDirection : uint8_t {
	LAST = 0,
	MAX_UP = 1,
	UP = 2,
	CENTER = 3,
	DOWN = 4,
	MAX_DOWN = 5,
};
enum class AirflowHorizontalDirection : uint8_t {
	LAST = 0,
	MAX_LEFT = 1,
	LEFT = 2,
	CENTER = 3,
	RIGHT = 4,
	MAX_RIGHT = 5,
};

class tclacClimate : public climate::Climate, public esphome::uart::UARTDevice, public PollingComponent {

	private:
		byte checksum;
		// dataTX with control consists of 38 bytes
		byte dataTX[38]{};
		// TCL status responses are known to use 61, 65, or 68 byte frames.
		// Keep enough room for the largest supported variant and validate the
		// declared length before accepting a frame.
		byte dataRX[68]{};
		size_t rx_pos_{0};
		size_t rx_expected_len_{0};
		uint32_t rx_last_byte_ms_{0};
		// Command to request status
		byte poll[8] = {0xBB,0x00,0x01,0x04,0x02,0x01,0x00,0xBD};
		byte energy_probe_query_[9] = {0xBB,0x00,0x01,0x0A,0x03,0x05,0x00,0x00,0xB6};
		// Initialization and initial filling of variables for switch states
		bool beeper_status_{false};
		bool display_status_{true};
		bool force_mode_status_{false};
		climate::ClimatePreset switch_preset{climate::CLIMATE_PRESET_NONE};
		bool module_display_status_{false};
		climate::ClimateFanMode switch_fan_mode{climate::CLIMATE_FAN_AUTO};
		bool is_call_control{false};
		climate::ClimateSwingMode switch_swing_mode{climate::CLIMATE_SWING_OFF};
		int target_temperature_set{7};
		climate::ClimateMode switch_climate_mode{climate::CLIMATE_MODE_OFF};
		bool allow_take_control{false};
		// HEAT transaction diagnostic only; does not change protocol/control behavior.
		bool heat_diag_pending_{false};
		uint32_t heat_diag_tx_ms_{0};
		
		esphome::climate::ClimateTraits traits_;
		
	public:

		tclacClimate() : PollingComponent(5 * 1000) {
			checksum = 0;
		}

		void readData();
		void takeControl();
		void loop() override;
		void setup() override;
		void update() override;
		void set_beeper_state(bool state);
		void set_display_state(bool state);
		void dataShow(bool flow, bool shine);
		void set_force_mode_state(bool state);
		void set_rx_led_pin(GPIOPin *rx_led_pin);
		void set_tx_led_pin(GPIOPin *tx_led_pin);
		void sendData(byte * message, byte size);
		void set_module_display_state(bool state);
		static String getHex(byte *message, byte size);
		void control(const ClimateCall &call) override;
		static byte getChecksum(const byte * message, size_t size);
		void set_vertical_airflow(AirflowVerticalDirection direction);
		void set_horizontal_airflow(AirflowHorizontalDirection direction);
		void set_vertical_swing_direction(VerticalSwingDirection direction);
		void set_horizontal_swing_direction(HorizontalSwingDirection direction);
		void set_supported_presets(climate::ClimatePresetMask presets);
		void set_supported_modes(climate::ClimateModeMask modes);
		void set_supported_fan_modes(climate::ClimateFanModeMask modes);
		void set_supported_swing_modes(climate::ClimateSwingModeMask modes);
		void set_dongle_uart(esphome::uart::UARTComponent *uart) { this->dongle_uart_ = uart; }
		void set_proxy_mode(bool enabled) { this->proxy_mode_ = enabled; }
		void set_a5_ack_only(bool enabled) { this->a5_ack_only_ = enabled; }
		void set_a5_raw_logger(bool enabled) { this->a5_raw_logger_enabled_ = enabled; }
		bool get_a5_raw_logger() const { return this->a5_raw_logger_enabled_; }
		void set_target_temperature_step(float step) { this->target_temperature_step_ = (step < 0.75f) ? 0.5f : 1.0f; }
		float get_target_temperature_step() const { return this->target_temperature_step_; }
		void a5_send_test_setpoint_24c();
		void a5_send_test_setpoint_25c();
		void a5_send_test_setpoint_26c();
		void a5_send_test_power_on();
		void a5_send_test_power_off();
		void a5_send_mode_(uint8_t mode);
		void a5_send_fan_(uint8_t speed);
		void a5_send_swing_(climate::ClimateSwingMode mode);
		void a5_send_preset_(climate::ClimatePreset preset);
		void a5_send_bool_field_(uint8_t field, bool enabled, const char *label);
		// Persistent AC beeper-policy setting (A5 field 0x25). This is not a momentary beep command.
		void a5_send_beep_setting_(bool enabled);
		void a5_send_setpoint_c_(float celsius);
		bool a5_display_light_valid() const { return this->a5_state_display_light_valid_; }
		bool a5_display_light() const { return this->a5_state_display_light_; }
		bool a5_beep_valid() const { return this->a5_state_beep_valid_; }
		bool a5_beep() const { return this->a5_state_beep_; }
		bool a5_health_valid() const { return this->a5_state_health_valid_; }
		bool a5_health() const { return this->a5_state_health_; }
		bool a5_drying_valid() const { return this->a5_state_drying_valid_; }
		bool a5_drying() const { return this->a5_state_drying_; }
		void set_swing_feature_enabled(bool enabled) { this->swing_feature_enabled_ = enabled; }
		void set_preset_feature_enabled(bool enabled) { this->preset_feature_enabled_ = enabled; }
		bool get_swing_feature_enabled() const { return this->swing_feature_enabled_; }
		bool get_preset_feature_enabled() const { return this->preset_feature_enabled_; }
		void a5_disable_swing_() { this->a5_send_swing_(climate::CLIMATE_SWING_OFF); }
		void a5_disable_preset_() { this->a5_send_preset_(climate::CLIMATE_PRESET_NONE); }
		void a5_publish_state_();
		void set_proxy_log_packets(bool enabled) { this->proxy_log_packets_ = enabled; }
		void set_proxy_idle_time(uint32_t ms) { this->proxy_idle_time_ms_ = ms; }
		void set_proxy_fallback_poll_interval(uint32_t ms) { this->proxy_fallback_poll_interval_ms_ = ms; }
		void set_humidity_entity_id(const std::string &entity_id);
		void set_external_humidity(float humidity) {
			if (!std::isfinite(humidity) || humidity < 0.0f || humidity > 100.0f) {
				this->clear_external_humidity();
				return;
			}
			this->current_humidity = humidity;
			this->publish_state();
		}
		void clear_external_humidity() {
			this->current_humidity = std::numeric_limits<float>::quiet_NaN();
			this->publish_state();
		}
		void set_error_code_sensor(sensor::Sensor *sensor) { this->error_code_sensor_ = sensor; }
		void set_room_temperature_sensor(sensor::Sensor *sensor) {
			this->room_temperature_sensor_ = sensor;
		}
		void set_indoor_coil_temperature_sensor(sensor::Sensor *sensor) {
			this->indoor_coil_temperature_sensor_ = sensor;
		}
		void set_indoor_coil_raw_sensor(sensor::Sensor *sensor) { this->indoor_coil_raw_sensor_ = sensor; }
		void set_pipe_out_temperature_sensor(sensor::Sensor *sensor) { this->pipe_out_temperature_sensor_ = sensor; }
		void set_pipe_in_temperature_sensor(sensor::Sensor *sensor) { this->pipe_in_temperature_sensor_ = sensor; }
		void set_outdoor_ambient_candidate_sensor(sensor::Sensor *sensor) {
			this->outdoor_ambient_candidate_sensor_ = sensor;
		}
		void set_outdoor_exhaust_candidate_sensor(sensor::Sensor *sensor) {
			this->outdoor_exhaust_candidate_sensor_ = sensor;
		}
		void set_outdoor_condenser_candidate_sensor(sensor::Sensor *sensor) {
			this->outdoor_condenser_candidate_sensor_ = sensor;
		}
		void set_compressor_frequency_candidate_sensor(sensor::Sensor *sensor) {
			this->compressor_frequency_candidate_sensor_ = sensor;
		}
		void set_external_temperature_candidate_raw_37_sensor(sensor::Sensor *sensor) {
			this->external_temperature_candidate_raw_37_sensor_ = sensor;
		}
		void set_external_temperature_candidate_raw_38_sensor(sensor::Sensor *sensor) {
			this->external_temperature_candidate_raw_38_sensor_ = sensor;
		}
		void set_compressor_current_sensor(sensor::Sensor *sensor) { this->compressor_current_sensor_ = sensor; }
		void set_compressor_state_sensor(sensor::Sensor *sensor) { this->compressor_state_sensor_ = sensor; }
		void set_airmax_fault_code_sensor(sensor::Sensor *sensor) { this->airmax_fault_code_sensor_ = sensor; }
		void set_raw_byte_45_sensor(sensor::Sensor *sensor) { this->raw_byte_45_sensor_ = sensor; }
		void set_active_supply_voltage_sensor(sensor::Sensor *sensor) { this->active_supply_voltage_sensor_ = sensor; }
		void set_a5_input_power_sensor(sensor::Sensor *sensor) { this->a5_input_power_sensor_ = sensor; }
		void set_a5_compressor_actual_sensor(sensor::Sensor *sensor) { this->a5_compressor_actual_sensor_ = sensor; }
		void set_a5_compressor_target_sensor(sensor::Sensor *sensor) { this->a5_compressor_target_sensor_ = sensor; }
		void set_outside_motor_sensor(sensor::Sensor *sensor) { this->outside_motor_sensor_ = sensor; }
		void set_vertical_vane_position_sensor(sensor::Sensor *sensor) { this->vertical_vane_position_sensor_ = sensor; }
		void set_horizontal_vane_position_sensor(sensor::Sensor *sensor) { this->horizontal_vane_position_sensor_ = sensor; }
		
	protected:
		GPIOPin *rx_led_pin_;
		GPIOPin *tx_led_pin_;
		ClimateTraits traits() override;
		climate::ClimateModeMask supported_modes_{};
		climate::ClimatePresetMask supported_presets_{};
		AirflowVerticalDirection vertical_direction_{AirflowVerticalDirection::LAST};
		climate::ClimateFanModeMask supported_fan_modes_{};
		AirflowHorizontalDirection horizontal_direction_{AirflowHorizontalDirection::LAST};
		VerticalSwingDirection vertical_swing_direction_{VerticalSwingDirection::UP_DOWN};
		climate::ClimateSwingModeMask supported_swing_modes_{};
		bool swing_feature_enabled_{true};
		bool preset_feature_enabled_{true};
		HorizontalSwingDirection horizontal_swing_direction_{HorizontalSwingDirection::LEFT_RIGHT};
		sensor::Sensor *error_code_sensor_{nullptr};
		sensor::Sensor *room_temperature_sensor_{nullptr};
		sensor::Sensor *indoor_coil_temperature_sensor_{nullptr};
		sensor::Sensor *indoor_coil_raw_sensor_{nullptr};
		sensor::Sensor *pipe_out_temperature_sensor_{nullptr};
		sensor::Sensor *pipe_in_temperature_sensor_{nullptr};
		sensor::Sensor *outdoor_ambient_candidate_sensor_{nullptr};
		sensor::Sensor *outdoor_exhaust_candidate_sensor_{nullptr};
		sensor::Sensor *outdoor_condenser_candidate_sensor_{nullptr};
		sensor::Sensor *compressor_frequency_candidate_sensor_{nullptr};
		sensor::Sensor *external_temperature_candidate_raw_37_sensor_{nullptr};
		sensor::Sensor *external_temperature_candidate_raw_38_sensor_{nullptr};
		sensor::Sensor *compressor_current_sensor_{nullptr};
		sensor::Sensor *compressor_state_sensor_{nullptr};
		sensor::Sensor *airmax_fault_code_sensor_{nullptr};
		sensor::Sensor *raw_byte_45_sensor_{nullptr};
		sensor::Sensor *active_supply_voltage_sensor_{nullptr};
		sensor::Sensor *a5_input_power_sensor_{nullptr};
		sensor::Sensor *a5_compressor_actual_sensor_{nullptr};
		sensor::Sensor *a5_compressor_target_sensor_{nullptr};
		sensor::Sensor *outside_motor_sensor_{nullptr};
		sensor::Sensor *vertical_vane_position_sensor_{nullptr};
		sensor::Sensor *horizontal_vane_position_sensor_{nullptr};
		std::string humidity_entity_id_{};

		// Optional inline proxy for the factory TCL Wi-Fi dongle.
		esphome::uart::UARTComponent *dongle_uart_{nullptr};
		bool proxy_mode_{false};
		bool a5_ack_only_{false};
		bool a5_raw_logger_enabled_{false};
		byte a5_frame_[128]{};
		size_t a5_frame_pos_{0};
		size_t a5_expected_len_{0};
		uint32_t a5_last_byte_ms_{0};
		bool a5_ack_pending_{false};
		uint32_t a5_ack_due_ms_{0};
		uint8_t a5_ack_link_{0};
		uint8_t a5_ack_counter_{0};
		uint8_t a5_ack_ptype_{0};
		uint32_t a5_valid_frames_{0};
		uint32_t a5_bad_crc_{0};
		uint32_t a5_acks_sent_{0};
		uint32_t a5_raw_bytes_{0};
		uint32_t a5_last_heartbeat_ms_{0};
		uint32_t a5_last_rx_report_ms_{0};
		bool a5_runtime_banner_logged_{false};
		// A5 dual-owner bridge: ESP32 remains the only AC-side protocol owner.
		// AC frames are mirrored to the factory dongle. Dongle ACK/service replies
		// are consumed locally; only valid 0A0A control commands are forwarded to AC.
		byte a5_dongle_frame_[128]{};
		size_t a5_dongle_frame_pos_{0};
		size_t a5_dongle_expected_len_{0};
		uint32_t a5_dongle_last_byte_ms_{0};
		uint32_t a5_dongle_raw_bytes_{0};
		uint32_t a5_dongle_valid_frames_{0};
		uint32_t a5_dongle_bad_crc_{0};
		uint32_t a5_dongle_commands_forwarded_{0};
		uint32_t a5_dongle_frames_suppressed_{0};
		uint32_t a5_last_rssi_ms_{0};
		uint32_t a5_clock_replies_sent_{0};
		bool a5_clock_pending_{false};
		uint32_t a5_rssi_sent_{0};
		uint32_t a5_commands_sent_{0};
		uint8_t a5_command_counter_{0x40};
		// Warm-rejoin/state-sync: reproduce the evidence-backed Factory Dongle
		// startup INIT 00 00 01 after ESP32 boot/restart so an already-powered AC
		// is prompted to publish its current A5 state again.
		bool a5_rejoin_started_{false};
		bool a5_rejoin_state_seen_{false};
		uint8_t a5_rejoin_attempts_{0};
		uint32_t a5_rejoin_next_ms_{0};
		bool a5_rejoin_query_pending_{false};
		bool a5_rejoin_query_sent_{false};
		uint32_t a5_rejoin_query_due_ms_{0};
		// Phase 3: cached values decoded from 0C0C state reports.  A5 reports are
		// deltas, so a field remains valid until the AC reports a newer value.
		bool a5_state_power_valid_{false}; bool a5_state_power_{false};
		bool a5_state_mode_valid_{false}; uint8_t a5_state_mode_{0};
		bool a5_state_setpoint_valid_{false}; float a5_state_setpoint_c_{NAN};
		float target_temperature_step_{1.0f};
		bool a5_state_room_valid_{false}; float a5_state_room_c_{NAN};
		bool a5_state_outdoor_valid_{false}; float a5_state_outdoor_c_{NAN};
		bool a5_state_fan_valid_{false}; uint8_t a5_state_fan_{0};
		bool a5_state_comp_target_valid_{false}; uint16_t a5_state_comp_target_{0};
		bool a5_state_comp_actual_valid_{false}; uint16_t a5_state_comp_actual_{0};
		bool a5_state_power_w_valid_{false}; uint16_t a5_state_power_w_{0};
		bool a5_state_coil_valid_{false}; float a5_state_coil_c_{NAN};
		bool a5_state_vertical_louver_valid_{false}; uint8_t a5_state_vertical_louver_{0x08};
		bool a5_state_horizontal_louver_valid_{false}; uint8_t a5_state_horizontal_louver_{0x08};
		bool a5_state_eco_valid_{false}; bool a5_state_eco_{false};
		bool a5_state_sleep_valid_{false}; uint8_t a5_state_sleep_{0};
		bool a5_state_display_light_valid_{false}; bool a5_state_display_light_{false};
		bool a5_state_beep_valid_{false}; bool a5_state_beep_{false};
		bool a5_state_health_valid_{false}; bool a5_state_health_{false};
		bool a5_state_drying_valid_{false}; bool a5_state_drying_{false};
		bool proxy_log_packets_{false};
		bool dual_phase_fast_{false};
		uint32_t dual_phase_boot_ms_{0};
		uint32_t proxy_idle_time_ms_{150};
		uint32_t proxy_fallback_poll_interval_ms_{15000};
		uint32_t proxy_last_activity_ms_{0};
		uint32_t proxy_last_dongle_activity_ms_{0};
		uint32_t proxy_last_fallback_poll_ms_{0};

		// Read-only CMD 0x0A energy probe. The TCL bus only answers this query
		// when it is chained shortly after a completed CMD 0x04 state response.
		bool energy_probe_pending_{false};
		uint32_t energy_probe_due_ms_{0};
		uint32_t energy_probe_last_query_ms_{0};
		uint32_t energy_probe_interval_ms_{30000};
		// Raw subtype 0x0D investigation state. Keep the previous payload so we
		// can report exactly which bytes changed without assigning semantics.
		byte energy_prev_payload_[45]{};
		bool energy_prev_payload_valid_{false};
		uint32_t energy_prev_rx_ms_{0};
		// Snapshot of the latest normal CMD 0x04 state, used only as context in
		// the energy-probe log. This does not feed Action v2 or HA entities.
		uint8_t energy_state_b38_{0};
		uint8_t energy_state_b39_{0};
		uint8_t energy_state_b40_{0};
		uint8_t energy_state_b45_{0};
		uint8_t energy_state_b46_{0};
		byte dongle_frame_[128]{};
		size_t dongle_frame_pos_{0};
		size_t dongle_frame_expected_len_{0};
		uint32_t dongle_frame_last_byte_ms_{0};
		byte pending_tx_[68]{};
		size_t pending_tx_len_{0};
		bool pending_tx_valid_{false};

		void a5_ack_loop_();
		void a5_parse_byte_(uint8_t value);
		void a5_process_frame_();
		void a5_dongle_parse_byte_(uint8_t value);
		void a5_dongle_process_frame_();
		void a5_send_ack_();
		void a5_send_clock_reply_();
		void a5_send_rssi_();
		void a5_send_rejoin_init_();
		void a5_send_rejoin_state_query_();
		void a5_send_frame_(uint8_t *f, size_t len, const char *label);
		void a5_decode_state_();
		void a5_log_state_();
		void a5_send_test_setpoint_(uint16_t centi_c, uint8_t degf);
		void a5_send_test_power_(bool on);
		uint16_t a5_crc16_(const uint8_t *data, size_t len);
		void proxy_forward_dongle_to_ac_();
		void proxy_parse_dongle_byte_(uint8_t value);
		void proxy_try_send_pending_();
		void proxy_queue_or_send_(const uint8_t *message, size_t size);
};
}
}

#endif //TCL_ESP_TCL_H
