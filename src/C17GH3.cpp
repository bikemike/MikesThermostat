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

void C17GH3State::processTx()
{
	static uint32_t timeNextSend = 0;
	static C17GH3MessageBase::C17GH3MessageType msgType = C17GH3MessageBase::MSG_TYPE_SETTINGS1;
	uint32_t timeNow = millis();
	if (timeNextSend < timeNow)
	{
		timeNextSend = timeNow + 1000; // 1 second

		C17GH3MessageQuery queryMsg(msgType);
		queryMsg.pack();
		Serial.write(queryMsg.getBytes(), 16);
		
		logger.addBytes("TX:", queryMsg.getBytes(), 16);

		
		if (C17GH3MessageBase::MSG_TYPE_SCHEDULE_DAY7 == msgType)
			msgType = C17GH3MessageBase::MSG_TYPE_SETTINGS1;
		else
			msgType = (C17GH3MessageBase::C17GH3MessageType)(msgType + 1);
	}
}

