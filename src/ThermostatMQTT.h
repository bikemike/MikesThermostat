#ifndef MQTT_H
#define MQTT_H

#include "ThermostatState.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

class ThermostatMQTT : public ThermostatState::Listener
{
public:
	ThermostatMQTT(ThermostatState* state);

	virtual void handleThermostatStateChange(const ThermostatState::ChangeEvent& c) override;
	void setName(String n) { name = n + "/"; } 
	void loop();
	void reconnect();

	static void static_callback(char* topic, byte* payload, unsigned int length);
	void callback(char* topic, byte* payload, unsigned int length);

	static ThermostatMQTT* self;

private:
	void subscribe();
	void sendHAMode();
	void sendHAAction();
private:
	String name = String("default/");
	ThermostatState* thermostatState;
	WiFiClient wifiClient;
	PubSubClient mqttClient;
	std::set<ThermostatState::ChangeEvent> pendingChangeEvents;

};

#endif // MQTT_H