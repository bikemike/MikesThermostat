#ifndef BHT002_H
#define BHT002_H

#include <Arduino.h>
#include "Log.h"
#include "ThermostatState.h"


extern Log logger;

/* MESSAGES:

header bytes:
0: 55  - sync byte
1: AA  - sync byte
2: 01  - version
3: xx  - command
4: 00  - length (HIGH)
5: 00  - length (LOW)
6 to length + 6 = message data
6 + length + 1 = checksum

command Word Description
0x00 Heartbeat detection.
0x01 Query product information.
0x02 Query the working mode of the Wi-Fi module.
0x03 Report the network connection status of the device.
0x04 Reset the Wi-Fi module and switch the network
configuration mode.
0x05 Reset the Wi-Fi module and select a network
configuration mode.
0x06 Deliver DP commands.
0x07 Report DP status.
0x08 Query the device initialization status.
0x0a (Optional) Start OTA upgrade.
0x0b (Optional) Transmit the OTA upgrade package.
0x1c (Optional) Obtain the local time.
0x0e Test the Wi-Fi function (product testing command).

heartbeat:
TX:55 aa 00 00 00 00 ff
RX: 55 aa 01 00 00 01 00 01
TX:55 aa 00 00 00 00 ff
RX: 55 aa 01 00 00 01 01 02

query parameters:

TX: 55 aa 00 08 00 00 07 
replies:
RX: 55 aa 01 07 00 05 01 01 00 01 01 10
RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 2e 45
RX: 55 aa 01 07 00 08 03 02 00 04 00 00 00 24 3c
RX: 55 aa 01 07 00 05 04 04 00 01 00 15
RX: 55 aa 01 07 00 05 05 01 00 01 00 13
RX: 55 aa 01 07 00 05 06 01 00 01 00 14
RX: 55 aa 01 07 00 3a 65 00 00 36 00 06 28 00 08 1e 1e b 1e 1e d 1e 00 11 2c 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e f
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
RX: 55 aa 01 07 00 05 68 01 00 01 01 77

TX: 55 aa 00 08 00 00 07
RX: 55 aa 01 07 00 05 01 01 00 01 01 10
RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 2e 45
RX: 55 aa 01 07 00 05 04 04 00 01 00 15
RX: 55 aa 01 07 00 05 05 01 00 01 00 13
RX: 55 aa 01 07 00 05 06 01 00 01 00 14
RX: 55 aa 01 07 00 3a 65 00 00 36 00 06 28 00 08 1e 1e b 1e 1e d 1e 00 11 2c 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e f
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
RX: 55 aa 01 07 00 05 68 01 00 01 01 77


status messages:
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
RX: 55 aa 01 07 00 08 03 02 00 04 00 00 00 24 3c

TX: 55 aa 00 02 00 00 01
RX: 55 aa 01 02 00 00 02

TX: 55 aa 00 03 00 00 02
RX: 55 aa 01 03 00 00 03

WIFI CONFIG:
RX: 55 aa 01 04 00 00 04

TX:55 aa 00 03 00 01 01 04

WIFI CONNECTED
TX: 55 aa 00 03 00 01 04 07
RX: 55 aa 01 03 00 00 03




0 = SYNC BYTE 1
1 = SYNC BYTE 2
2 = VERSION
3 = COMMAND
4 = PAYLOAD LENGTH HIGH 
5 = PAYLOAD LENGTH LOW
6 = PAYLOAD MESSAGE ID

 = CHECKSUM


 heating  23.5
TX: 55 aa 00 08 00 00 07
RX: 55 aa 01 07 00 05 01 01 00 01 01 10
RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 2f 46
RX: 55 aa 01 07 00 05 04 04 00 01 01 16
RX: 55 aa 01 07 00 05 05 01 00 01 00 13
RX: 55 aa 01 07 00 05 06 01 00 01 00 14
RX: 55 aa 01 07 00 3a 65 00 00 36 00 06 28 00 08 1e 1e b 1e 1e d 1e 00 11 2c 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e f
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
RX: 55 aa 01 07 00 05 68 01 00 01 01 77

RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 2f 46 - set temp
RX: 55 aa 01 07 00 08 03 02 00 04 00 00 00 2a 42 - therm temp
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b - floor temp


TX: 55 aa 00 08 00 00 07
RX: 55 aa 01 07 00 05 01 01 00 01 01 10
RX: 55 aa 01 07 00 08 02 02 00 04 00 00 00 29 40
RX: 55 aa 01 07 00 05 04 04 00 01 01 16
RX: 55 aa 01 07 00 05 05 01 00 01 00 13
RX: 55 aa 01 07 00 05 06 01 00 01 00 14
RX: 55 aa 01 07 00 3a 65 00 00 36 00 06 28 00 08 1e 1e b 1e 1e d 1e 00 11 2c 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e 00 06 28 00 08 28 1e b 28 1e d 28 00 11 28 00 16 1e f
RX: 55 aa 01 07 00 08 66 02 00 04 00 00 00 00 7b
RX: 55 aa 01 07 00 05 68 01 00 01 01 77

not heading 20.5
*/
class TUYAMessage : public MessageInterface
{
public:

	enum TUYAMessageCmd
	{
		MSG_CMD_HEARTBEAT = 0x00,
		MSG_CMD_QUERY_PROD_INFO,
		MSG_CMD_QUERY_WIFI_MODE,
		MSG_CMD_REPORT_WIFI_STATUS,
		MSG_CMD_RESET_WIFI_SWITCH_NET_CFG,
		MSG_CMD_RESET_WIFI_SELECT_NET_CFG,
		MSG_CMD_DP_CMD, // DP = data point
		MSG_CMD_DP_STATUS,
		MSG_CMD_QUERY_DEVICE_STATUS, // request all DPs
		MSG_CMD_START_OTA_UPGRADE = 0x0a,
		MSG_CMD_TRANSMIT_OTA_PACKAGE = 0x0b,
		MSG_CMD_OBTAIN_LOCAL_TIME = 0x1c,
		MSG_CMD_TEST_WIFI = 0x0e,	
	};



public:

	TUYAMessage(TUYAMessageCmd _cmd)
	{
		magic1 = 0x55;
		magic2 = 0xaa; 
		version = 0;
		cmd = _cmd;
		setPayloadLength(0);
		pack();
	}
	
	TUYAMessage(const uint8_t* _bytes, uint32_t length)
	{
		if (length <= MAX_BUFFER_LENGTH)
		{
			memcpy(bytes,_bytes,length);
			messageLength = length;
		}
	}
	virtual ~TUYAMessage(){}

	bool operator==(TUYAMessage& other)
	{
		for (unsigned int i = 0; i < messageLength; ++i)
		{
			if (bytes[i] != other.bytes[i])
				return false;
		}
		return true;
	}

	void setBytes(const uint8_t* srcbytes, uint32_t length)
	{
		if (length <= MAX_BUFFER_LENGTH)
		{
			messageLength = length;
			memcpy(bytes, srcbytes, length);
		}
	}

	const uint8_t* getBytes() const
	{
		return bytes;
	}
	uint32_t getLength() const
	{
		return messageLength;
	}

	virtual bool isValid() const override
	{
		return (magic1 == 0x55 && magic2 == 0xaa && messageLength >= 7 && calcChecksum() == bytes[messageLength-1]);
	}

	virtual String toString()
	{
		return String("TUYA MSG");
	}

	void pack()
	{
		setCheckusm();
	}

	void setCommand(TUYAMessageCmd _cmd)
	{
		cmd = _cmd;
	}
	TUYAMessageCmd getCommand() const
	{
		return (TUYAMessageCmd)cmd;
	}

	uint8_t getVersion() const
	{
		return version;
	}
	void setVersion(uint8_t v)
	{
		version = v;
	}

	uint16_t getPayloadLength() const
	{
		return payload_length_hi * 0x100 + payload_length_lo;
	}

	void setPayloadLength(uint16_t length)
	{
		payload_length_hi = length >> 8;
		payload_length_lo = length & 0xff;
		messageLength = length + 7;
	}

	const uint8_t* getPayload() const { return &payload; }


	void setPayload(const uint8_t* payload, uint32_t length)
	{
		if (length <= MAX_BUFFER_LENGTH - 7)
		{
			setPayloadLength(length);
			memcpy(bytes+6, payload, length);
		}
	}
	
	uint8_t& magic1 = bytes[0];
	uint8_t& magic2 = bytes[1];
	uint8_t& version = bytes[2];
	uint8_t& cmd = bytes[3];
	uint8_t& payload_length_hi = bytes[4];
	uint8_t& payload_length_lo = bytes[5];
	uint8_t& payload = bytes[6];

protected:
	static const uint16_t MAX_BUFFER_LENGTH = 260;
	uint8_t bytes[MAX_BUFFER_LENGTH] = {0};
	uint32_t messageLength = 0;

	uint8_t calcChecksum() const
	{
		uint16_t chksum = 0;
		for (unsigned int i = 0 ; i < messageLength -1; ++i)
			chksum += bytes[i];

		chksum %= 256;
		return (uint8_t)chksum;
	}
	
	void setCheckusm()
	{
		bytes[messageLength-1] = calcChecksum();
	}



};


class TUYAMessageBuffer
{
public:
	bool addbyte(uint8_t byte)
	{
		uint32_t newLastMS = millis();
	
		if (newLastMS - lastMS > 5)
			reset();

		checkReset();

		lastMS = newLastMS;

		if (0 == currentByte)
		{
			if (0x55 == byte)
			{
				bytes[currentByte] = byte;
				++currentByte;
			}
				
		}
		else if (1 == currentByte)
		{
			if (0xAA == byte)
			{
				bytes[currentByte] = byte;
				++currentByte;
			}
			else
			{
				reset();
			}
		}
		else
		{
			bytes[currentByte] = byte;

			if (5 == currentByte)
					dataLength = bytes[4] * 0x100 + bytes[5];

			if (currentByte >= 6u && dataLength+6u == currentByte)
			{
				reset();
				return true;
			}
			else
				++currentByte;
		}
		return false;
	}
	const uint8_t* getBytes() const
	{
		return bytes;
	}

	const uint32_t getLength()
	{
		return 6 + dataLength + 1;
	}

	void reset()
	{
		resetBuffer = true;
	}
	void checkReset()
	{
		if (resetBuffer)
		{
			currentByte = 0;
			dataLength = 0;
			resetBuffer = false;
		}
	}
private:
	static const uint16_t MAX_BUFFER_LENGTH = 260;

	uint8_t  bytes[MAX_BUFFER_LENGTH] = {0};
	uint32_t currentByte = 0;
	uint16_t dataLength  = 0;  
	uint32_t lastMS      = 0;
	bool     resetBuffer = false;
	
};


class TUYAThermostatState : public ThermostatState
{
public:
enum WifiMode
{
	WIFI_MODE_COOPERATIVE_PROCESSING,
	WIFI_MODE_WIFI_PROCESSING
};

enum WifiState
{
	WIFI_STATE_SMART_CONFIG,
	WIFI_STATE_AP_CONFIG,
	WIFI_STATE_CONNECT_FAILED,
	WIFI_STATE_CONNECTED,
	WIFI_STATE_CONNECTED_WITH_INTERNET,
	WIFI_STATE_LOW_POWER
};


public:
	//C17GH3State::C17GH3State() {}
	virtual void loop();
	virtual void processRx() override;
	virtual void processRx(int byte) override;
	virtual void processTx(bool timeAvailable = false) override;
	virtual void sendMessage(const MessageInterface& msg) const override;

	typedef std::function<void()> WifiConfigCallback;
	void setWifiConfigCallback(WifiConfigCallback cb)
	{
		wifiConfigCallback = cb;
	}

	void setPower(const bool on, bool updateMCU = false) override;
	void setSetPointTemp(const float temp, bool updateMCU = false) override;
	void setInternalTemp(const float temp) override;
	void setExternalTemp(const float temp) override;
	void setMode(const Mode mode, bool updateMCU = false) override;
	void setEconomy(const bool econ, bool updateMCU = false) override;
	void setLock(const bool lock, bool updateMCU = false) override;


	bool getPower() const override { return powerOn; }
	float getSetPointTemp() const override { return setPointTemp; }
	float getInternalTemp() const override { return internalTemp; }
	float getExternalTemp() const override { return externalTemp; }
	Mode getMode() const override{ return mode; }
	bool getEconomy() const override { return economyOn; }
	bool getLock() const override { return locked; }

	virtual void setSchedule(const uint8_t* schedule, uint8_t length, bool updateMCU = false) override;
	virtual int getScheduleDay() const;
	virtual int getSchedulePeriod() const;
	virtual float getScheduleCurrentPeriodSetPointTemp() const;
	virtual float getScheduleSetPointTemperature(int day, int period) const;

	String toString()
	{
		String str;
		str += "HEATING: ";
		if (getIsHeating())
			str  += "ON\n";
		else
			str += "OFF\n";
		str +=
		    String("SetTemp: ") + String(getSetPointTemp()) +
		    String(", internal temp: ") + String(getInternalTemp()) +
		    String(", external temp: ") + String(getExternalTemp()) +
		    String(", lock: ") + String(getLock()) + 
		    String(", mode: ") + String(getMode()) +
		    String(", power: ") + String(getPower()) +
		    String(", economy: ") + String(getEconomy()) +
			String("\n\n") +
			String("Schedule:");
		for (int i = 0; i < 3; ++i)
		{
			if (0 == i)
				str += String("\nM-F: ");
			else if (1 == i)
				str += String("\nSat: ");
			else if (2 == i)
				str += String("\nSun: ");
			for(int j = 0; j < 6; ++j)
			{
				uint8_t m = schedule[i * 18 + j * 3 + 0];
				uint8_t h = schedule[i * 18 + j * 3 + 1];
				float t = schedule[i * 18 + j * 3 + 2] / 2.f;
				if (0 != j)
					str += ", ";
				str += String("time") + String(j+1) + String(": ");
				str += String(h) + String(":") + String(m);
				str += String(" temp") + String(j+1) + String(": ");
				str += String(t);
			}
		}

		str += "\n";
		str += "curr day:";
		str += String(scheduleCurrentDay);
		str += ", curr period:";
		str += String(scheduleCurrentPeriod);
		str += ", curr temp:";
		str += String(getScheduleCurrentPeriodSetPointTemp());
		return str;
	}
private:
	struct SchedulePeriod
	{
		int hour = 0;
		int minute = 0;
		float temperature = 0;
	};
	void sendHeartbeat() const;
	void updateWifiState();
	void sendTime(bool timeAvailable) const;
	void setWifiState(WifiState newState);
	void handleDPStatusMsg(const TUYAMessage& msg);
	void getSchedulePeriod(int day, int period, SchedulePeriod &p) const;


	TUYAMessageBuffer msgBuffer;

	WifiConfigCallback wifiConfigCallback;
	bool gotHeartbeat = false;
	bool gotProdKey = false;
	bool gotWifiMode = false;
	bool canQuery    = false;
	bool sendWifiStateMsg = false;
	mutable uint8_t protocolVersion = 0;
	WifiMode wifiMode = WIFI_MODE_COOPERATIVE_PROCESSING;
	WifiState wifiState = WIFI_STATE_CONNECT_FAILED;


	bool powerOn = false;
	float setPointTemp = 20.f;
	float internalTemp = 20.f;
	float externalTemp = 20.f;
	Mode mode = MODE_SCHEDULE;
	bool economyOn = false;
	bool locked = false;
	uint8_t schedule[54] = {0};

	bool haveSchedule = false;
	int scheduleCurrentDay = -1;
	int scheduleCurrentPeriod = -1;

};

#endif