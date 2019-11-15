# MikesThermostat 
Alternative firmware for the Wifi modules of the following thermostats:

* Floureon C17.GH3 WiFi Thermostat
* MoesHouse BHT-002-GBLW Thermostat

This code only replaces the firmware on the wifi module of the thermostat. It allows for non-cloud control of your wifi thermostat through MQTT (ie Home Assistant).

Home Assistant Config Example:
```
climate:
  - platform: mqtt
    name: Livingroom Thermostat
    modes:
      - "off"
      - "heat"
      - "auto"
    availability_topic: "Thermostat-d63a9a/availability"
    action_topic: "Thermostat-d63a9a/ha_action"
    mode_command_topic: "Thermostat-d63a9a/ha_mode/set"
    temperature_state_topic: "Thermostat-d63a9a/setpoint_temp"
    temperature_command_topic: "Thermostat-d63a9a/setpoint_temp/set"
    current_temperature_topic: "Thermostat-d63a9a/internal_temp"
    mode_state_topic: "Thermostat-d63a9a/ha_mode"
    min_temp: 12
    max_temp: 26
    temp_step: 0.5
    precision: 0.5
    retain: false
```
