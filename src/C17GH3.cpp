#include <ESP8266WiFi.h> 
#include <sys/time.h>

#include "C17GH3.h"
#include "Log.h"

extern Log logger;


void C17GH3State::loop()
{

	processRx();
	processTx(getTimeAvailable());

	static uint32_t timeLastSend = 0;
	uint32_t timeNow = millis();
	if (timeNow - timeLastSend > 50)
	{
		timeLastSend = timeNow;
		sendMessages();
	}

	if (getTimeAvailable() && haveSchedule)
	{
		// update scheduleCurrentDay  and scheduleCurrentPeriod
		static uint32_t timeLastScheduleUpdate = timeNow+1;
		if (timeNow - timeLastScheduleUpdate > 30000)
		{
			timeLastScheduleUpdate = timeNow;
			int day = 0;
			int hour = 0;
			int mins = 0;
			if (getTime(day, hour, mins))
			{
				// make monday first day
				if (day == 0)
					day = 7;
				--day;

				int period = -1;
				for (int i = 0 ; i < 6; ++i)
				{
					if (hour < schedule[day].getHour(i))
						break;
					else if (hour == schedule[day].getHour(i) && mins < schedule[day].getMinute(i))
						break;
					else
						period = i;
				}


				if (-1 == period) // still on previous day schedule
				{
					if (day == 0)
						day = 7;
					--day;
					period = 5;
				}
				if (day != scheduleCurrentDay || period != scheduleCurrentPeriod)
				{
					scheduleCurrentDay = day;
					scheduleCurrentPeriod = period;
					emitChange(ChangeEvent::CHANGE_TYPE_CURRENT_SCHEDULE_PERIOD);
				}
			}
		}
	}
}

int C17GH3State::getScheduleDay() const
{
	return scheduleCurrentDay;
}

int C17GH3State::getSchedulePeriod() const
{
	return scheduleCurrentPeriod;
}

float C17GH3State::getScheduleCurrentPeriodSetPointTemp() const
{
	return getScheduleSetPointTemperature(scheduleCurrentDay, scheduleCurrentPeriod );
}

float C17GH3State::getScheduleSetPointTemperature(int day, int period) const
{
	float f = 0;
	if (day >= 0 && day < 7 && period >= 0 && period < 6)
		f = schedule[day].getTemperature(period);
	return f;
}



void C17GH3State::sendMessages()
{
	// send 1
	if (!pendingMessages.empty())
	{
		auto itr = pendingMessages.begin();
		Serial.write(itr->getBytes(), 16);
		logger.addBytes("TX:", itr->getBytes(), 16);

		pendingMessages.erase(itr);
	}
}


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
				C17GH3MessageSettings1 s1Old(settings1);
				settings1.setBytes(msg.getBytes());

				logger.addLine(settings1.toString());

				if (s1Old.getExternalTemperature() != settings1.getExternalTemperature())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_EXTERNAL_TEMP);
				}
				if (s1Old.getInternalTemperature() != settings1.getInternalTemperature())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_INTERNAL_TEMP);
				}
				if (s1Old.getLock() != settings1.getLock())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_LOCK);
				}
				if (s1Old.getMode() != settings1.getMode())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_MODE);
				}
				if (s1Old.getPower() != settings1.getPower())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_POWER);
				}
				if (s1Old.getSetPointTemp() != settings1.getSetPointTemp())
				{
					emitChange(ChangeEvent::CHANGE_TYPE_SETPOINT_TEMP);
				}
			}
		}
		break;
		case 0xC2:
		{
			C17GH3MessageSettings2 s2Old = settings2;
			settings2.setBytes(msg.getBytes());
			logger.addLine(settings2.toString());
		}
		break;
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
			{
				schedule[msg.type - 0xC3].setBytes(msg.getBytes());
				logger.addLine(schedule[msg.type - 0xC3].toString());
				if (!haveSchedule)
				{
					bool valid = true;
					for (int i = 0; i < 7 && valid; ++i)
					{
						valid = schedule[i].isValid();
					}
					if (valid)
						haveSchedule = true;
				}
			}
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


void C17GH3State::processTx(bool timeAvailable /* = false*/)
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

void C17GH3State::sendMessage(const MessageInterface& msg) const
{
	C17GH3MessageBase m(msg.getBytes());
	auto ib = pendingMessages.insert(m);
	if (!ib.second)
	{
		pendingMessages.erase(ib.first);
		pendingMessages.insert(m);
	}
}


C17GH3MessageSettings1::WiFiState C17GH3State::getWiFiState() const
{
	return settings1.getWiFiState();
}

int C17GH3State::getPWM() const
{
	return pwm;
}

void C17GH3State::setPWM(int _pwm)
{
	if (pwm != _pwm)
	{
		pwm = _pwm; 
	}
}

bool C17GH3State::getLock() const
{
	return settings1.getLock();
}

void C17GH3State::setLock(bool locked, bool updateMCU)
{
	if (locked != getLock())
	{
		settings1.setLock(locked);
		if (updateMCU)
		{
			C17GH3MessageSettings1 msg;
			msg.setBytes(settings1.getBytes());
			msg.setTxFields(false);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_LOCK);
	}
	
}
 
ThermostatState::Mode C17GH3State::getMode() const
{
	return settings1.getMode() ? MODE_MANUAL : MODE_SCHEDULE;
}

void C17GH3State::setMode(Mode mode_, bool updateMCU)
{
	if (mode_ != getMode())
	{
		settings1.setMode(mode_ == MODE_MANUAL ? true : false);
		if (updateMCU)
		{
			C17GH3MessageSettings1 msg;
			msg.setBytes(settings1.getBytes());
			msg.setTxFields(false);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_MODE);
	}
}
 
bool C17GH3State::getPower() const
{
	return settings1.getPower();
}

void C17GH3State::setPower(bool pow, bool updateMCU) 
{
	if (pow != getPower())
	{
		settings1.setPower(pow);

		if (updateMCU)
		{
			C17GH3MessageSettings1 msg;
			msg.setBytes(settings1.getBytes());
			msg.setTxFields(false);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_POWER);
	}
}

float C17GH3State::getSetPointTemp() const
{
	return settings1.getSetPointTemp();
}

void C17GH3State::setSetPointTemp(float temperature, bool updateMCU)
{
	if (temperature != getSetPointTemp())
	{
		settings1.setSetPointTemp(temperature);

		if (updateMCU)
		{
			C17GH3MessageSettings1 msg;
			msg.setBytes(settings1.getBytes());
			msg.setTxFields(false);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_SETPOINT_TEMP);
	}
}

float C17GH3State::getInternalTemp() const
{
	return settings1.getInternalTemperature();
}

float C17GH3State::getExternalTemp() const
{
	return settings1.getExternalTemperature();
}


bool C17GH3State::getBacklightMode() const
{
	return settings2.getBacklightMode();
}

void C17GH3State::setBacklightMode(bool bl, bool updateMCU)
{
	if (bl != getBacklightMode())
	{
		settings2.setBacklightMode(bl);

		C17GH3MessageSettings2 msg;
		msg.setBytes(settings2.getBytes());
		msg.pack();
		sendMessage(msg);
	}
}

bool C17GH3State::getPowerOnMode() const
{
	return settings2.getPowerOnMode();
}

void C17GH3State::setPowerOnMode(bool pm, bool updateMCU)
{
	if (pm != getPowerOnMode())
	{
		settings2.setPowerOnMode(pm);

		C17GH3MessageSettings2 msg;
		msg.setBytes(settings2.getBytes());
		msg.pack();
		sendMessage(msg);
	}
}

bool C17GH3State::getAntifreezeMode() const
{
	return settings2.getAntifreezeMode();
}

void C17GH3State::setAntifreezeMode(bool am, bool updateMCU)
{
	if (am != getAntifreezeMode())
	{
		settings2.setAntifreezeMode(am);

		C17GH3MessageSettings2 msg;
		msg.setBytes(settings2.getBytes());
		msg.pack();
		sendMessage(msg);
	}
}

C17GH3MessageSettings2::SensorMode C17GH3State::getSensorMode() const
{
	return settings2.getSensorMode();
}

void C17GH3State::setSensorMode(C17GH3MessageSettings2::SensorMode sm)
{
	if (sm != getSensorMode())
	{
		settings2.setSensorMode(sm);

		C17GH3MessageSettings2 msg;
		msg.setBytes(settings2.getBytes());
		msg.pack();
		sendMessage(msg);
	}
}

