#include <ESP8266WiFi.h> 
#include <sys/time.h>

#include "BHT002.h"


void TUYAThermostatState::loop()
{
	processRx();
	processTx(getTimeAvailable());

	// update scheduleCurrentDay  and scheduleCurrentPeriod
	if (getTimeAvailable())
	{
		uint32_t timeNow = millis();
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
				SchedulePeriod p;
				for (int i = 0 ; i < 6; ++i)
				{
					getSchedulePeriod(day, i, p);

					if (hour < p.hour)
					{
						break;
					}
					else if (hour == p.hour && mins < p.minute)
					{
						break;
					}
					else
					{
						period = i;
					}
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


int TUYAThermostatState::getScheduleDay() const
{
	return scheduleCurrentDay;
}

int TUYAThermostatState::getSchedulePeriod() const
{
	return scheduleCurrentPeriod;
}

float TUYAThermostatState::getScheduleCurrentPeriodSetPointTemp() const
{
	return getScheduleSetPointTemperature(scheduleCurrentDay, scheduleCurrentPeriod );
}

float TUYAThermostatState::getScheduleSetPointTemperature(int day, int period) const
{
	SchedulePeriod p;
	getSchedulePeriod(day, period, p);
	return p.temperature;
}

void TUYAThermostatState::getSchedulePeriod(int day, int period, SchedulePeriod &p) const
{
	if (day < 0 || day > 6 || period < 0 || period > 5)
		return;

	int i = 0;
	int j = period;
	if (i > 4)
		i = day - 4;

	p.minute = schedule[i * 18 + j * 3 + 0];
	p.hour = schedule[i * 18 + j * 3 + 1];
	p.temperature = schedule[i * 18 + j * 3 + 2] / 2.f;
}


void TUYAThermostatState::processRx()
{
	if (Serial.available())
	{
		while (Serial.available())
		{
			processRx(Serial.read());
		}
	}
}

void TUYAThermostatState::processRx(int byte)
{
	bool hasMsg = msgBuffer.addbyte(byte);
	if (hasMsg)
	{
		//processRx(TUYAMessage(msgBuffer.getBytes()));
		TUYAMessage msg(msgBuffer.getBytes(), msgBuffer.getLength());
		if (!msg.isValid())
		{
			logger.addBytes("RX INVALID MSG:", msgBuffer.getBytes(), msgBuffer.getLength());
			return;
		}
		logger.addBytes("RX:", msgBuffer.getBytes(), msgBuffer.getLength());
		switch(msg.getCommand())
		{
		case TUYAMessage::MSG_CMD_HEARTBEAT:
			{
				if (1 == msg.getPayloadLength())
				{
					if (1 != msg.getPayload()[0])
						sendWifiStateMsg = true;
					if (!gotHeartbeat)
					{
						gotHeartbeat = true;
						protocolVersion = msg.getVersion();

						if (!gotProdKey)
						{
							TUYAMessage msg(TUYAMessage::MSG_CMD_QUERY_PROD_INFO);
							sendMessage(msg);
						}
					}
				}
			}
			break;
		case TUYAMessage::MSG_CMD_QUERY_PROD_INFO:
			{
				if (msg.getPayloadLength())
				{
					gotProdKey = true;

					if (!gotWifiMode)
					{
						TUYAMessage msg(TUYAMessage::MSG_CMD_QUERY_WIFI_MODE);
						sendMessage(msg);
					}
				}
			}
			break;
		case TUYAMessage::MSG_CMD_QUERY_WIFI_MODE:
			{
				gotWifiMode = true;
				if (msg.getPayloadLength() == 2)
				{
					//const uint8_t* payload = msg.getPayload();
					//uint8_t wifi_indicator_pin = payload[0];
					//uint8_t reset_pin = payload[1];
					wifiMode = WIFI_MODE_WIFI_PROCESSING;
				}
				else
				{
					wifiMode = WIFI_MODE_COOPERATIVE_PROCESSING;					
				}
				logger.addLine("WIFI PROCESSING MODE:" + String(wifiMode));

			}
			break;
		case TUYAMessage::MSG_CMD_REPORT_WIFI_STATUS:
			{
				canQuery = true;
				TUYAMessage msg(TUYAMessage::MSG_CMD_QUERY_DEVICE_STATUS);
				sendMessage(msg);
			}
			break;
		case TUYAMessage::MSG_CMD_RESET_WIFI_SWITCH_NET_CFG:
			{
				TUYAMessage msg(TUYAMessage::MSG_CMD_RESET_WIFI_SWITCH_NET_CFG);
				sendMessage(msg);

				if (wifiConfigCallback)
				{
					setWifiState(WIFI_STATE_SMART_CONFIG);
					wifiConfigCallback();
				}
			}
			break;
		case TUYAMessage::MSG_CMD_RESET_WIFI_SELECT_NET_CFG:
			{
				// not implemented

			}
			break;
		case TUYAMessage::MSG_CMD_DP_STATUS:
			{
				if (msg.getPayloadLength())
				{
					handleDPStatusMsg(msg);
				}
			}
			break;
		default:
			// unknown command
			break;
		}
	}
}

void TUYAThermostatState::handleDPStatusMsg(const TUYAMessage& msg)
{
	const uint8_t* payload = msg.getPayload();

	switch(payload[0])
	{
	case 0x01: // power on/off (byte 4)
		{
			//RX: 55 aa 01 07 00 05 01 01 00 01 01 10
			if (5 == msg.getPayloadLength()	)
			{
				setPower(1 == payload[4]);
			}
		}
		break;
	case 0x02: // set point temp
		{
			//RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 2e 45
			if (8 == msg.getPayloadLength())
			{
				setSetPointTemp(payload[7]/2.0f);
			}

		}
		break;
	case 0x03: // temperature
		{
			//RX: 55 aa 01 07 00 08 03 02 00 04 00 00 00 24 3c
			if (8 == msg.getPayloadLength())
			{
				setInternalTemp(payload[7]/2.0f);
			}
		}
		break;
	case 0x04: // mode (schedule = 0 / manual = 1)
		{
			//RX: 55 aa 01 07 00 05 04 04 00 01 00 15
			if (5 == msg.getPayloadLength())
			{
				setMode(payload[4] ? MODE_MANUAL : MODE_SCHEDULE);

			}

		}
		break;
	case 0x05: // economy
		{
			//RX: 55 aa 01 07 00 05 05 01 00 01 00 13
			if (5 == msg.getPayloadLength())
			{
				setEconomy(1 == payload[4]);
			}
		}
		break;
	case 0x06: // lock
		{
			//RX: 55 aa 01 07 00 05 06 01 00 01 00 14
    		//    TX: 55 AA 00 06 00 05 06 01 00 01 01 00


			if (5 == msg.getPayloadLength())
			{
				setLock(1 == payload[4]);
			}
		}
		break;
	case 0x65: // schedule
		{
			// RX: 55 aa 01 07 00 3a 65
			// 00 00 36 00
			// 06 28 00 08 1e 1e b 1e 1e d 1e 00 11 2c 00 16 1e 00
			// 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e 00
			// 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e f
			if (58  == msg.getPayloadLength())
			{
				setSchedule(payload + 4, msg.getPayloadLength()-4);
			}
		}
		break;
	case 0x66: // floor temp
		{
			// RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
			if (8 == msg.getPayloadLength())
			{
				setExternalTemp(payload[7]/2.0f);
			}
		}
		break;
	case 0x68: // ??
		{
			// RX: 55 aa 01 07 00 05 68 01 00 01 01 77
			if (5 == msg.getPayloadLength())
			{
			}

		}
		break;

	}

}

void TUYAThermostatState::processTx(bool timeAvailable /* = false*/)
{
	sendHeartbeat();
	updateWifiState();
	sendTime(timeAvailable);


}

void TUYAThermostatState::sendHeartbeat() const
{
	static uint32_t timeLastSend = 0;
	static uint32_t delay = 3000;
	uint32_t timeNow = millis();
	
	if (timeNow - timeLastSend > delay)
	{
		timeLastSend = timeNow;
		if (gotHeartbeat)
			delay = 10000;
		else
			delay = 3000;

		TUYAMessage msg(TUYAMessage::MSG_CMD_HEARTBEAT);
		sendMessage(msg);
	}
}

void TUYAThermostatState::setWifiState(WifiState newState)
{
	if (wifiState != newState || sendWifiStateMsg)
	{
		wifiState = newState;

		sendWifiStateMsg = true;
		
		if (gotWifiMode)
		{
			TUYAMessage msg(TUYAMessage::MSG_CMD_REPORT_WIFI_STATUS);
			uint8_t payload[1] = {(uint8_t)wifiState};
			msg.setPayload(payload,1);
			msg.pack();
			sendMessage(msg);
			sendWifiStateMsg = false;
		}

	}

}

void TUYAThermostatState::updateWifiState()
{
	// check once per second for wifi state change
	static uint32_t timeLastSend = 0;
	uint32_t timeNow = millis();
	
	if (timeNow - timeLastSend > 1000)
	{
		timeLastSend = timeNow;

		WifiState newState = WIFI_STATE_CONNECT_FAILED;
		wl_status_t wifiStatus = WiFi.status();
		switch(wifiStatus)
		{
			case WL_NO_SHIELD:
			case WL_IDLE_STATUS:
			case WL_NO_SSID_AVAIL:
			case WL_DISCONNECTED:
			case WL_CONNECTION_LOST:
			case WL_CONNECT_FAILED:
				newState = WIFI_STATE_CONNECT_FAILED;
				break;
			case WL_SCAN_COMPLETED:
				//newWifiState = settings1.getWiFiState();
				break;
			case WL_CONNECTED:
				newState = WIFI_STATE_CONNECTED_WITH_INTERNET;
				break;
		}
		
		setWifiState(newState);
	}
}

void TUYAThermostatState::sendTime(bool timeAvailable) const
{
	struct tm* new_time = nullptr;
	if (timeAvailable)
	{
		struct timezone tz = {0};
		timeval tv = {0};
		
		gettimeofday(&tv, &tz);
		time_t tnow = time(nullptr);

		// localtime / gmtime every second change
		static time_t nexttv = 0;
		if (nexttv < tv.tv_sec && canQuery)
		{
			nexttv = tv.tv_sec + 3600; // update every hour
			new_time = localtime(&tnow);
		}
	}

	if (new_time != nullptr)
	{
		// weekday: 0 = monday, 6 = sunday
		uint8_t w = (new_time->tm_wday == 0 ? 7 : new_time->tm_wday);
		uint8_t s = new_time->tm_sec;
		uint8_t m = new_time->tm_min;
		uint8_t h = new_time->tm_hour;
		uint8_t d = new_time->tm_mday;
		uint8_t M = new_time->tm_mon  + 1;
		uint8_t y = new_time->tm_year % 100;

		TUYAMessage msg(TUYAMessage::MSG_CMD_OBTAIN_LOCAL_TIME);
		uint8_t payload[8] = {01,y,M,d,h,m,s,w};
		msg.setPayload(payload, 8);
		msg.pack();
		sendMessage(msg);
	}
}

void TUYAThermostatState::sendMessage(const MessageInterface& msg) const
{
	if (msg.isValid())
	{
		Serial.write(msg.getBytes(), msg.getLength());
		logger.addBytes("TX:", msg.getBytes(), msg.getLength());
	}
	else
	{
		logger.addBytes("TX INVALID MSG:", msg.getBytes(), msg.getLength());

	}
}

void TUYAThermostatState::setPower(const bool on, bool updateMCU)
{
	if (on != powerOn)
	{
		powerOn = on;
		if (updateMCU)
		{
			// send change to mcu
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[5] = {0x01, 0x01, 0x00, 0x01, (uint8_t)(powerOn ? 1 : 0)};
			msg.setPayload(payload, 5);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_POWER);
	}

}
void TUYAThermostatState::setSetPointTemp(const float temp, bool updateMCU)
{
	if (temp != setPointTemp)
	{
		setPointTemp = temp;
		if (updateMCU)
		{
			// send change to mcu
			//RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 24 3c
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[8] = {0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, (uint8_t)(setPointTemp*2 + 0.5)};
			msg.setPayload(payload, 8);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_SETPOINT_TEMP);
	}
	
}
void TUYAThermostatState::setInternalTemp(const float temp)
{
	if (temp != internalTemp)
	{
		internalTemp = temp;
		emitChange(ChangeEvent::CHANGE_TYPE_INTERNAL_TEMP);
	}
}
void TUYAThermostatState::setExternalTemp(const float temp)
{
	if (temp != externalTemp)
	{
		externalTemp = temp;
		emitChange(ChangeEvent::CHANGE_TYPE_EXTERNAL_TEMP);
	}
	
}
void TUYAThermostatState::setMode(const Mode m, bool updateMCU)
{
	if (m != mode)
	{
		mode = m;
		if (updateMCU)
		{
			// send change to mcu
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[5] = {0x04, 0x01, 0x00, 0x01, mode};
			msg.setPayload(payload, 5);
			msg.pack();
			sendMessage(msg);
		}
		emitChange(ChangeEvent::CHANGE_TYPE_MODE);
	}
}
void TUYAThermostatState::setEconomy(const bool econ, bool updateMCU)
{
	if (econ != economyOn)
	{
		economyOn = econ;
		if(updateMCU)
		{
			// send change to mcu
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[5] = {0x05, 0x01, 0x00, 0x01,(uint8_t)( economyOn ? 1 : 0)};
			msg.setPayload(payload, 5);
			msg.pack();
			sendMessage(msg);			
		}
		emitChange(ChangeEvent::CHANGE_TYPE_ECONOMY);
	}
}
void TUYAThermostatState::setLock(const bool lock, bool updateMCU)
{
	if (lock != locked)
	{
		locked = lock;
		if(updateMCU)
		{
			// send change to mcu	
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[5] = {0x06, 0x01, 0x00, 0x01, (uint8_t)(locked ? 1 : 0)};
			msg.setPayload(payload, 5);
			msg.pack();
			sendMessage(msg);			
		}
		emitChange(ChangeEvent::CHANGE_TYPE_LOCK);
	}	
}


void TUYAThermostatState::setSchedule(const uint8_t* s, uint8_t length, bool updateMCU)
{
	if (54 != length)
		return;

	bool changed = false;

	//setSchedule weekday, sat, sun x 6 times
	for (int i = 0; i < length; ++i)
	{
		if (s[i] != schedule[i])
		{
			schedule[i] = s[i];
			changed = true;
		}
	}
	if (changed)
	{
		haveSchedule = true;
		if (updateMCU)
		{
			// send change to mcu
			TUYAMessage msg(TUYAMessage::MSG_CMD_DP_CMD);
			uint8_t payload[58] = {0x65, 0x00, 0x00, 0x36, 0};
			uint8_t* ptr = payload + 4;
			for (int i = 0; i < 54; ++i)
			{
				ptr[i] = schedule[i];
			}
			msg.setPayload(payload, 58);
			msg.pack();
			sendMessage(msg);			

		}
	}
}