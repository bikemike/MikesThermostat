#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include "ThermostatState.h"


class Webserver : public ThermostatState::Listener
{
public:
	Webserver() {}
	void init(class ThermostatState* state, String deviceName);
	void stop();
	void start();

	void handleRoot();
	void handleNotFound();
	void handleConsole();
	void handleRestart();
	void process();

	virtual void handleThermostatStateChange(const ThermostatState::ChangeEvent& c) override;

private:
	String deviceName;
	ESP8266WebServer* server = nullptr;
	ThermostatState* state = nullptr;
	ESP8266HTTPUpdateServer* httpUpdater = nullptr;

};
#endif
