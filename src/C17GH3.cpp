#include "C17GH3.h"

void C17GH3State::processRx(const C17GH3MessageBase& msg)
{
	if (!msg.isValid())
	{
		Serial.println("ERROR: Invalid MSG");
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
				Serial.println("WIFI CONFIG REQUEST");
				if (wifiConfigCallback)
				{
					wifiConfigCallback();
				}
			}
			else
			{	
				settings1.setBytes(msg.getBytes());
				Serial.println(settings1.toString());
			}
		}
		break;
		case 0xC2:
			settings2.setBytes(msg.getBytes());
			Serial.println(settings2.toString());
		break;
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
			schedule[msg.type - 0xC3].setBytes(msg.getBytes());
			Serial.println(schedule[msg.type - 0xC3].toString());
		break;
		default:
			Serial.println("MSG Not handled");
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
		//Serial.write(queryMsg.getBytes(), 16);
		
		Serial.print("TX:");
		for (int i = 0 ; i < 16; ++i)
		{
			Serial.printf(" %02X",queryMsg.getBytes()[i]);
		}
		Serial.println();

		
		if (C17GH3MessageBase::MSG_TYPE_SCHEDULE_DAY7 == msgType)
			msgType = C17GH3MessageBase::MSG_TYPE_SETTINGS1;
		else
			msgType = (C17GH3MessageBase::C17GH3MessageType)(msgType + 1);
	}
}

