#include <ESP8266WiFi.h> 
#include <sys/time.h>

#include "C17GH3.h"
#include "Log.h"

extern Log logger;

void C17GH3State::processRx()
{
	if (Serial.available())
	{
		while (Serial.available())
		{
			processRx(Serial.read());
		}
	}
}

void C17GH3State::processRx(int byte)
{
	bool hasMsg = msgBuffer.addbyte(byte);
	if (hasMsg)
	{
		processRx(C17GH3MessageBase(msgBuffer.getBytes()));
	}
}

void C17GH3State::processRx(const C17GH3MessageBase& msg)
{
	logger.addBytes("RX:", msg.getBytes(), 16);

	if (!msg.isValid())
	{
		logger.addLine("ERROR: Invalid MSG");
		return;
	}
	switch(msg.type)
	{
		case 0xC1:
		{
			C17GH3MessageSettings1 s1msg;
			s1msg.setBytes(msg.getBytes());
			if (C17GH3MessageSettings1::WIFI_STATE_CONFIG == s1msg.getWiFiState())
			{
				// wifi config request
				logger.addLine("WIFI CONFIG REQUEST");
				if (wifiConfigCallback)
				{
					wifiConfigCallback();
				}
			}
			else
			{	
				settings1.setBytes(msg.getBytes());
				logger.addLine(settings1.toString());
			}
		}
		break;
		case 0xC2:
			settings2.setBytes(msg.getBytes());
			logger.addLine(settings2.toString());
		break;
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
			schedule[msg.type - 0xC3].setBytes(msg.getBytes());
			logger.addLine(schedule[msg.type - 0xC3].toString());
		break;
		default:
			logger.addLine("MSG Not handled");
		break;
	}
}

bool C17GH3State::isValidState(const C17GH3MessageBase::C17GH3MessageType &msgType) const
{
	if (msgType == settings1.type)
		return settings1.isValid();
	else if (msgType == settings2.type)
		return settings2.isValid();
	else
		for (int i = 0 ; i < 7; ++i)
			if (msgType == schedule[i].type)
				return schedule[i].isValid();
	return false;
}

void C17GH3State::sendSettings1(bool timeAvailable) const
{
	static uint32_t nextSend = 0;
	uint32_t millisNow = millis();
	if (millisNow < nextSend)
		return;

	if (!settings1.isValid())
		return;

	nextSend = millisNow + 1000;

	C17GH3MessageSettings1::WiFiState newWifiState = C17GH3MessageSettings1::WIFI_STATE_DISCONNECTED;
	wl_status_t wifiStatus = WiFi.status();
	switch(wifiStatus)
	{
		case WL_NO_SHIELD:
		case WL_IDLE_STATUS:
		case WL_NO_SSID_AVAIL:
		case WL_DISCONNECTED:
		case WL_CONNECTION_LOST:
		case WL_CONNECT_FAILED:
			newWifiState = C17GH3MessageSettings1::WIFI_STATE_DISCONNECTED;
			break;
		case WL_SCAN_COMPLETED:
			newWifiState = settings1.getWiFiState();
			break;
		case WL_CONNECTED:
			newWifiState = C17GH3MessageSettings1::WIFI_STATE_CONNECTED;
			break;
	}

	struct tm* new_time = nullptr;
	if (timeAvailable)
	{
		struct timezone tz = {0};
		timeval tv = {0};
		
		gettimeofday(&tv, &tz);
		time_t tnow = time(nullptr);

		// localtime / gmtime every second change
		static time_t nexttv = 0;
		if (nexttv < tv.tv_sec)
		{
			nexttv = tv.tv_sec + 3600; // update every hour
			new_time = localtime(&tnow);
		}
	}

	if (newWifiState != settings1.getWiFiState() ||
		new_time != nullptr )
	{
		if (newWifiState != settings1.getWiFiState())
			logger.addLine(String("Wifi state: ") + String(newWifiState) + String("Old:") + String(settings1.getWiFiState()));

		C17GH3MessageSettings1 msg;
		msg.setBytes(settings1.getBytes());
		msg.setWiFiState(newWifiState);
		msg.setTxFields(false);
		if (new_time != nullptr)
		{
			// wday = 1-7 = mon - sun
			msg.setDayOfWeek(new_time->tm_wday == 0 ? 7 : new_time->tm_wday);
			msg.setHour(new_time->tm_hour);
			msg.setMinute(new_time->tm_min);
		}
		msg.pack();
		sendMessage(msg);
	}
}

void C17GH3State::sendSettings2() const
{

}

void C17GH3State::processTx(bool timeAvailable /* = false*/) const
{
	static uint32_t timeNextSend = 0;
	uint32_t timeNow = millis();
	static C17GH3MessageBase::C17GH3MessageType msgType = C17GH3MessageBase::MSG_TYPE_SETTINGS1;
 
	if (timeNextSend < timeNow)
	{
		if (firstQueriesDone)
			timeNextSend = timeNow + 12000;
		else
			timeNextSend = timeNow + 300;
	
		C17GH3MessageQuery queryMsg(msgType);
		queryMsg.pack();
		sendMessage(queryMsg);
		
		if (C17GH3MessageBase::MSG_TYPE_SCHEDULE_DAY7 == msgType)
		{
			msgType = C17GH3MessageBase::MSG_TYPE_SETTINGS1;
			firstQueriesDone = true;
		}
		else
			msgType = (C17GH3MessageBase::C17GH3MessageType)(msgType + 1);
	}

	
	sendSettings1(timeAvailable);
	sendSettings2();

}

void C17GH3State::sendMessage(const C17GH3MessageBase& msg) const
{
	Serial.write(msg.getBytes(), 16);
	logger.addBytes("TX:", msg.getBytes(), 16);
}


C17GH3MessageSettings1::WiFiState C17GH3State::getWiFiState() const
{
	return settings1.getWiFiState();
}

bool C17GH3State::getLock() const
{
	return settings1.getLock();
}

void C17GH3State::setLock(bool locked)
{
	C17GH3MessageSettings1 msg;
	msg.setBytes(settings1.getBytes());
	msg.setTxFields(false);
	msg.setLock(locked);
	msg.pack();
	sendMessage(msg);
}
 
bool C17GH3State::getMode() const
{
	return settings1.getMode();
}

void C17GH3State::setMode(bool mode_)
{
	C17GH3MessageSettings1 msg;
	msg.setBytes(settings1.getBytes());
	msg.setTxFields(false);
	msg.setMode(mode_);
	msg.pack();
	sendMessage(msg);
}
 
bool C17GH3State::getPower() const
{
	return settings1.getPower();
}

void C17GH3State::setPower(bool pow) 
{
	C17GH3MessageSettings1 msg;
	msg.setBytes(settings1.getBytes());
	msg.setTxFields(false);
	msg.setPower(pow);
	msg.pack();
	sendMessage(msg);
}

float C17GH3State::getSetPointTemp() const
{
	return settings1.getSetPointTemp();
}

void C17GH3State::setSetPointTemp(float temperature)
{
	C17GH3MessageSettings1 msg;
	msg.setBytes(settings1.getBytes());
	msg.setTxFields(false);
	msg.setSetPointTemp(temperature);
	msg.pack();
	sendMessage(msg);
}

float C17GH3State::getInternalTemperature() const
{
	return settings1.getInternalTemperature();
}

float C17GH3State::getExternalTemperature() const
{
	return settings1.getExternalTemperature();
}


bool C17GH3State::getBacklightMode() const
{
	return settings2.getBacklightMode();
}

void C17GH3State::setBacklightMode(bool bl)
{
	C17GH3MessageSettings2 msg;
	msg.setBytes(settings2.getBytes());
	msg.setBacklightMode(bl);
	msg.pack();
	sendMessage(msg);
}

bool C17GH3State::getPowerMode() const
{
	return settings2.getPowerMode();
}

void C17GH3State::setPowerMode(bool pm)
{
	C17GH3MessageSettings2 msg;
	msg.setBytes(settings2.getBytes());
	msg.setPowerMode(pm);
	msg.pack();
	sendMessage(msg);
}

bool C17GH3State::getAntifreezeMode() const
{
	return settings2.getAntifreezeMode();
}

void C17GH3State::setAntifreezeMode(bool am)
{
	C17GH3MessageSettings2 msg;
	msg.setBytes(settings2.getBytes());
	msg.setAntifreezeMode(am);
	msg.pack();
	sendMessage(msg);
}

C17GH3MessageSettings2::SensorMode C17GH3State::getSensorMode() const
{
	return settings2.getSensorMode();
}

void C17GH3State::setSensorMode(C17GH3MessageSettings2::SensorMode sm)
{
	C17GH3MessageSettings2 msg;
	msg.setBytes(settings2.getBytes());
	msg.setSensorMode(sm);
	msg.pack();
	sendMessage(msg);
}

