#include "ThermostatMQTT.h"
#include "Log.h"
extern Log logger;
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define mqtt_server "192.168.31.107" // Enter your MQTT server adderss or IP. I use my DuckDNS adddress (yourname.duckdns.org) in this field
#define mqtt_user "" //enter your MQTT username
#define mqtt_password "" //enter your password

#define topic_availability "availability"
#define topic_power "power"   // on/off
#define topic_aux_heat "aux_heat"
#define topic_mode "mode" // 1 = manual, 0 = auto
#define topic_economy "economy"
#define topic_setpoint_temp "setpoint_temp"
#define topic_internal_temp "internal_temp"
#define topic_external_temp "external_temp"
#define topic_is_heating "is_heating"
#define topic_schedule "schedule"
#define topic_lock "lock"

// topics specific for home assistant mqtt climate platform
#define topic_ha_action "ha_action"  // idle heating off
#define topic_ha_mode   "ha_mode"    // auto off heat



ThermostatMQTT* ThermostatMQTT::self = nullptr;


ThermostatMQTT::ThermostatMQTT(class ThermostatState* state) : 
	thermostatState(state)
{
	self = this;

	mqttClient.setClient(wifiClient);
	mqttClient.setServer(mqtt_server, 1883); //CHANGE PORT HERE IF NEEDED
	mqttClient.setCallback(static_callback);

	if (thermostatState)
	{
		thermostatState->addListener(this);
	}
}

void ThermostatMQTT::sendHAMode()
{
	String value = "off";

	// off heat auto
	if (thermostatState->getPower())
	{
		if (thermostatState->getMode() == ThermostatState::MODE_MANUAL)
		{
			value = "heat";
		}
		else
		{
			value = "auto";
		}
	}
		
	mqttClient.publish((name+topic_ha_mode).c_str(), value.c_str(), true);

}

void ThermostatMQTT::sendHAAction()
{
	// idle heating off
	String value = "off";
	if (thermostatState->getPower())
	{
		if (thermostatState->getIsHeating())
		{
			value = "heating";
		}
		else
		{
			value = "idle";
		}
	}
	mqttClient.publish((name+topic_ha_action).c_str(), value.c_str(), true);
	
}


void ThermostatMQTT::handleThermostatStateChange(const ThermostatState::ChangeEvent& c)
{
	if (!mqttClient.connected())
	{
		pendingChangeEvents.insert(c);
		return;
	}

	bool published = false;
	switch(c.getType())
	{
	case ThermostatState::ChangeEvent::CHANGE_TYPE_POWER:
		{
			published = mqttClient.publish((name+topic_power).c_str(), thermostatState->getPower() ? "ON" : "OFF", true);
			sendHAMode();
			sendHAAction();
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_MODE:
		{
			published = mqttClient.publish((name+topic_mode).c_str(), thermostatState->getMode() == ThermostatState::MODE_MANUAL ? "1" : "0", true);
			if (thermostatState->getMode() == ThermostatState::MODE_MANUAL)
			{
				published = mqttClient.publish((name+topic_setpoint_temp).c_str(), String(thermostatState->getSetPointTemp()).c_str(), true);
			}
			else
			{
				published = mqttClient.publish((name+topic_setpoint_temp).c_str(), String(thermostatState->getScheduleCurrentPeriodSetPointTemp()).c_str(), true);
			}
			
			sendHAMode();
			sendHAAction();
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_ECONOMY:
		{
			published = mqttClient.publish((name+topic_economy).c_str(), thermostatState->getEconomy() ? "ON" : "OFF", true);
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_SETPOINT_TEMP:
		{
			if (thermostatState->getMode() == ThermostatState::MODE_MANUAL)
			{
				published = mqttClient.publish((name+topic_setpoint_temp).c_str(), String(thermostatState->getSetPointTemp()).c_str(), true);
			}
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_INTERNAL_TEMP:
		{
			published = mqttClient.publish((name+topic_internal_temp).c_str(), String(thermostatState->getInternalTemp()).c_str());
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_EXTERNAL_TEMP:
		{
			published = mqttClient.publish((name+topic_external_temp).c_str(), String(thermostatState->getExternalTemp()).c_str());
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_IS_HEATING:
		{
			published = mqttClient.publish((name+topic_is_heating).c_str(), thermostatState->getIsHeating() ? "ON" : "OFF", true);
			sendHAAction();
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_SCHEDULE:
		//published = mqttClient.publish((name+topic_schedule).c_str(), thermostatState->getPower() ? "ON" : "OFF");
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_LOCK:
		{
			published = mqttClient.publish((name+topic_lock).c_str(), thermostatState->getLock() ? "ON" : "OFF", true);
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_AUX_HEAT_ENABLED:
		{
			published = mqttClient.publish((name+topic_aux_heat).c_str(), thermostatState->getAuxHeatEnabled() ? "ON" : "OFF");
		}
		break;
	case ThermostatState::ChangeEvent::CHANGE_TYPE_CURRENT_SCHEDULE_PERIOD:
		{
			if (thermostatState->getMode() == ThermostatState::MODE_SCHEDULE)
			{
				published = mqttClient.publish((name+topic_setpoint_temp).c_str(), String(thermostatState->getScheduleCurrentPeriodSetPointTemp()).c_str(), true);
			}
		}
		break;
	default:
		break;
	}
}


void ThermostatMQTT::loop()
{
	static uint32_t lastServiceTime = 0;
	static uint32_t lastServiceTimeMQTT = 0;

	uint32_t now = millis();
	if (now - lastServiceTime > 1000)
	{
		lastServiceTime = now;
		if (mqttClient.connected() && !pendingChangeEvents.empty())
		{
			ThermostatState::ChangeEvent c = *pendingChangeEvents.begin();
			pendingChangeEvents.erase(pendingChangeEvents.begin());
			handleThermostatStateChange(c);
		}
	}


	if (!mqttClient.connected())
	{
		reconnect();
	}
	else
	{
		// slow down mqtt messages received to
		// limit number of messages to mcu
		if (now - lastServiceTimeMQTT > 25) 
		{
			lastServiceTimeMQTT = now;
			mqttClient.loop();
		}
	}

}

void ThermostatMQTT::reconnect()
{
	static uint32_t lastConnectAttempt = 0;

	uint32_t now = millis();

	if (now - lastConnectAttempt > 1000)
	{
		lastConnectAttempt = now;
		// Loop until we're reconnected
		if (!mqttClient.connected())
		{
			String s = "MQTT Connect...";

			// Attempt to connect
			if (mqttClient.connect(
				name.c_str(), 
				mqtt_user, mqtt_password,
				(name+topic_availability).c_str(),
				0, true, "offline"))
			{
				s += "connected";
				mqttClient.publish((name+topic_availability).c_str(),"online", true);
				subscribe();
			}
			else
			{
				s += "fail, rc=";
				s += String(mqttClient.state());
			}   

			logger.addLine(s);
		}
	}
}


void ThermostatMQTT::static_callback(char* topic, byte* payload, unsigned int length)
{
	if (self)
		self->callback(topic,payload,length);
}

void ThermostatMQTT::subscribe()
{
	mqttClient.subscribe((name + topic_power"/set").c_str());
	mqttClient.subscribe((name + topic_aux_heat"/set").c_str());
	mqttClient.subscribe((name + topic_mode"/set").c_str());
	mqttClient.subscribe((name + topic_economy"/set").c_str());
	mqttClient.subscribe((name + topic_setpoint_temp"/set").c_str());
	mqttClient.subscribe((name + topic_schedule"/set").c_str());
	mqttClient.subscribe((name + topic_lock"/set").c_str());
	mqttClient.subscribe((name + topic_ha_mode"/set").c_str());
}

void ThermostatMQTT::callback(char* topic_, byte* payload, unsigned int length)
{
	char msg_buff[64];
	
	if (length >= 63)
		return;


	memcpy(msg_buff, payload, length);
	msg_buff[length] = 0;
	String msg(msg_buff);
	String topic(topic_);

	logger.addLine(String("MQTT: ") + topic + ": " + msg);


	if (topic.endsWith(topic_power"/set"))
	{
		if (msg.equalsIgnoreCase("on"))
			thermostatState->setPower(1, true);
		else if (msg.equalsIgnoreCase("off"))
			thermostatState->setPower(0, true);
	}
	else if (topic.endsWith(topic_ha_mode"/set"))
	{
		if (msg.equalsIgnoreCase("off"))
			thermostatState->setPower(false, true);
		else if (msg.equalsIgnoreCase("heat"))
		{
			thermostatState->setPower(true, true);
			thermostatState->setMode(ThermostatState::MODE_MANUAL, true);
		}
		else if (msg.equalsIgnoreCase("auto"))
		{
			thermostatState->setPower(true, true);
			thermostatState->setMode(ThermostatState::MODE_SCHEDULE, true);
		}
	}
	else if (topic.endsWith(topic_mode"/set"))
	{
		if (msg.equalsIgnoreCase("1"))
			thermostatState->setMode(ThermostatState::MODE_MANUAL, true);
		else if (msg.equalsIgnoreCase("0"))
			thermostatState->setMode(ThermostatState::MODE_SCHEDULE, true);
	}
	else if (topic.endsWith(topic_economy"/set"))
	{
		if (msg.equalsIgnoreCase("on"))
			thermostatState->setEconomy(1, true);
		else if (msg.equalsIgnoreCase("off"))
			thermostatState->setEconomy(0, true);
	}
	else if (topic.endsWith(topic_setpoint_temp"/set"))
	{
		if (thermostatState->getMode() == ThermostatState::MODE_MANUAL)
		{
			thermostatState->setSetPointTemp(msg.toFloat(), true);
		}
	}
	else if (topic.endsWith(topic_aux_heat"/set"))
	{
		if (msg.equalsIgnoreCase("on"))
			thermostatState->setAuxHeatEnabled(true);
		else if (msg.equalsIgnoreCase("off"))
			thermostatState->setAuxHeatEnabled(false);
	}
	else if (topic.endsWith(topic_schedule"/set"))
	{
	}
	else if (topic.endsWith(topic_lock"/set"))
	{
		if (msg.equalsIgnoreCase("on"))
			thermostatState->setLock(1, true);
		else if (msg.equalsIgnoreCase("off"))
			thermostatState->setLock(0, true);
	}
}




