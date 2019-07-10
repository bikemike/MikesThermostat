#ifndef C17GH3_H
#define C17GH3_H

#include <Arduino.h>


class C17GH3MessageBase
{
public:
	C17GH3MessageBase(){}
	
	C17GH3MessageBase(const uint8_t* _bytes)
	{
		memcpy(bytes,_bytes,16);
	}
	virtual ~C17GH3MessageBase(){}

	bool operator==(C17GH3MessageBase& other)
	{
		for (int i = 0; i < 16; ++i)
		{
			if (bytes[i] != other.bytes[i])
				return false;
		}
		return true;
	}

	void setBytes(const uint8_t* srcbytes)
	{
		memcpy(bytes, srcbytes, 16);
	}

	const uint8_t* getBytes() const
	{
		return bytes;
	}

	bool isValid() const
	{
		return (magic1 == 0xaa && magic2 == 0x55 && calcChecksum() == checksum);
	}

	virtual String toString()
	{
		return String("Msg Type: ") + String(type,HEX);
	}

	void pack()
	{
		setCheckusm();
	}
	
	const uint8_t& magic1 = bytes[0];
	const uint8_t& magic2 = bytes[1];
	uint8_t& type = bytes[2];
	uint8_t& checksum = bytes[15];

	enum C17GH3MessageType
	{
		MSG_TYPE_QUERY = 0xC0,
		MSG_TYPE_SETTINGS1,
		MSG_TYPE_SETTINGS2,
		MSG_TYPE_SCHEDULE_DAY1, // MONDAY
		MSG_TYPE_SCHEDULE_DAY2, // TUESDAY
		MSG_TYPE_SCHEDULE_DAY3, // WEDNESDAY
		MSG_TYPE_SCHEDULE_DAY4, // THURSDAY
		MSG_TYPE_SCHEDULE_DAY5, // FRIDAY
		MSG_TYPE_SCHEDULE_DAY6, // SATURDAY
		MSG_TYPE_SCHEDULE_DAY7, // SUNDAY
	};


protected:
	uint8_t bytes[16] = {0xAA,0x55,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uint8_t calcChecksum() const
	{
		uint16_t chksum = 0;
		for (int i = 0 ; i < 15; ++i)
			chksum += bytes[i];

		chksum %= 256;
		return (uint8_t)chksum;
	}
	
	void setCheckusm()
	{
		checksum = calcChecksum();
	}


};

class C17GH3MessageQuery : public C17GH3MessageBase
{
public:
	C17GH3MessageQuery(C17GH3MessageType msgType)
	{
		type = MSG_TYPE_QUERY;
		query = (uint8_t)msgType;
	}
	uint8_t &query = bytes[3];

	virtual String toString()
	{
		return String("Query MSG Type: ") + String(query,HEX);
	}
};

class C17GH3MessageSettings1 : public C17GH3MessageBase
{
public:
	C17GH3MessageSettings1()
	{
		type = (uint8_t)MSG_TYPE_SETTINGS1;
	}

	virtual String toString(bool sending = false)
	{
		String s;
		s = String("Settings 1: WifiState: ") + String(getWiFiState()) +
		    String(", SetTemp: ") + String(getSetPointTemp()) +
		    String(", lock: ") + String(getLock()) + 
		    String(", mode: ") + String(getMode()) +
		    String(", power: ") + String(getPower());

		if (sending)
		{
			s += 
			    String(", day: ") + String(getDayOfWeek()) + 
		    	String(", hour: ") + String(getHour()) + 
			    String(", minute: ") + String(getMinute());
		}
		else
		{
 			s += String(", internal temp: ") + String(getInternalTemperature()) + 
		    	String(", external temp: ") + String(getExternalTemperature());
		}
		return s;
	}

	enum WiFiState
	{
		WIFI_STATE_CONFIG,
		WIFI_STATE_UNKNOWN1,
		WIFI_STATE_DISCONNECTED,
		WIFI_STATE_CONNECTING,
		WIFI_STATE_DISCONNECTING,
		WIFI_STATE_CONNECTED,
		WIFI_STATE_UNKNOWN2
	};

	void setWiFiState(WiFiState state)
	{
		wifi_state = (uint8_t)state;
	}
	WiFiState getWiFiState() const
	{
		return (WiFiState)wifi_state;
	}

	bool getLock() const
	{
		return 0xFF == lock;
	}
	void setLock(bool locked) 
	{
		lock = (locked ? 0xFF : 0);
	}
	bool getMode() const
	{
		return 0 != mode;
	}
	void setMode(bool mode_) 
	{
		mode = (mode_ ? 0xFF : 0);
	}
	bool getPower() const
	{
		return 0 != power;
	}
	void setPower(bool pow) 
	{
		power = (pow ? 1 : 0);
	}

	float getSetPointTemp() const
	{
		return set_point_temp/2.f;
	}
	void setSetPointTemp(float temperature)
	{
		temperature = std::max(std::min(temperature,30.f),0.f);
		set_point_temp = (uint8_t)(temperature * 2 + .5f); 
	}

	uint8_t getDayOfWeek() const
	{
		return day_of_week;
	}
	void setDayOfWeek(uint8_t dow)
	{
		if (dow > 7)
			dow = 7;
		if (dow < 1)
			dow = 1;
		day_of_week = dow;
	}
	uint8_t getHour() const
	{
		return hour;
	}
	void setHour(uint8_t h)
	{
		if (h > 23)
			h = 23;
		hour = h;
	}
	uint8_t getMinute() const
	{
		return minute;
	}
	void setMinute(uint8_t m)
	{
		if (m > 59)
			m = 59;
		minute = m;
	}

	void setTxFields(bool hasTime)
	{
		if (!hasTime)
		{
			day_of_week = 0xff;
			hour = 0xff;
			minute = 0xff;
		}
		unknown7   = 0xff;
		unknown9  = 0xff;
		unknown10 = 0xff;
		unknown11 = 0xff;
	}

	float getInternalTemperature() const
	{
		uint32_t temp = temperature_internal_high << 8;
		temp += temperature_internal_low;
		return float(temp) / 10.f;
	}
	float getExternalTemperature() const
	{
		uint32_t temp = temperature_external_high << 8;
		temp += temperature_external_low;
		return float(temp) / 10.f;
	}

	uint8_t &wifi_state = bytes[3];  // 00 = start wifi hotspot, 02 = disconnected, 03 = connecting, 04 = disconnecting?, 05 = connected , 06
	
	// when sending to MCU, the following 4 bytes set the time
	uint8_t &day_of_week = bytes[4]; // FF = don't set 1-7
	uint8_t &hour        = bytes[5]; // FF = don't set
	uint8_t &minute      = bytes[6]; // FF = don't set
	uint8_t &unknown7    = bytes[7];  // FF = don't set

	// when receiving, the following 4 bytes are internal and external temperature
    // temp: 00 F2 == 242 == 24.2, 01 01 == 261 == 26.1
	uint8_t &temperature_internal_high  = bytes[4];
	uint8_t &temperature_internal_low   = bytes[5];
	uint8_t &temperature_external_high  = bytes[6];
	uint8_t &temperature_external_low   = bytes[7];

	uint8_t &set_point_temp   = bytes[8];  // 1E 2f 2b 2d = 30 43 47 49
	uint8_t &unknown9   = bytes[9];  // 00 ( set unknown1-3 to FF when sending time/wifistate to mcu)
	uint8_t &unknown10   = bytes[10]; // 00
	uint8_t &unknown11   = bytes[11]; // 00
	uint8_t &lock       = bytes[12]; // lock = ff, unlocked = 00
	uint8_t &mode       = bytes[13]; // manual = ff, program mode = 00
	uint8_t &power      = bytes[14]; // thermostat on=1/off=0
};


class C17GH3MessageSettings2 : public C17GH3MessageBase
{
public:
	C17GH3MessageSettings2()
	{
		type = (uint8_t)MSG_TYPE_SETTINGS2;
	}
	uint8_t &backlightMode       = bytes[3];  // 00 = autooff, ff = steady on
	uint8_t &powerMode           = bytes[4];  // 00 = off when powered on, ff == last state of power
	uint8_t &antifreezeMode      = bytes[5];     // 00 = off, ff = on 
	uint8_t &tempCorrection      = bytes[6]; // -5 to +5, default -2
	uint8_t &internalHysteresis  = bytes[7]; // 0x32 = 50 = 5 degrees , .5 to 5 degrees, .5 increments
	uint8_t &externalHysteresis  = bytes[8]; // 1e  = 30 = 3.0 degrees
	uint8_t &unknown9            = bytes[9]; // 00
	uint8_t &sensorMode          = bytes[10];  // 00 = internal, 01 = external, 02 = both 
	uint8_t &externalSensorLimit = bytes[11]; // 40-80degrees. default 55
	uint8_t &unknown12           = bytes[12]; // 00
	uint8_t &unknown13           = bytes[13]; // 00 // ff - 03
	uint8_t &unknown14           = bytes[14]; // 01

	// TEMPERATURE CORRECTION -5 to 5 degrees (default -2)
	// could be : (unknown2, unknown4, unknown6, unknown7, unknown8) 
	
	//dt0 internal sensor hysteresis .5 to 4 degrees (default 1 )
	// could be : (unknown8) 
	
	//dt1 external sensor hysteresis .5 to 5 degrees (default 3)
	// could be : (unknown2, unknown8)

	// external temperature sensor limit 40 to 80 degrees (default 55) (all or external
	// could be : (unknown5)
	enum SensorMode
	{
		SENSOR_MODE_INTERNAL,
		SENSOR_MODE_EXTERNAL,
		SENSOR_MODE_BOTH,
	};

	bool getBacklightMode() const
	{
		return 0 != backlightMode;
	}
	void setBacklightMode(bool bl)
	{
		backlightMode = bl ? 0xFF : 0;
	}

	bool getPowerMode() const
	{
		return 0 != powerMode;
	}
	void setPowerMode(bool pm)
	{
		powerMode = pm ? 0xFF : 0;
	}

	bool getAntifreezeMode() const
	{
		return 0 != antifreezeMode;
	}
	void setAntifreezeMode(bool am)
	{
		antifreezeMode = am ? 0xFF : 0;
	}

	SensorMode getSensorMode() const
	{
		return (SensorMode)sensorMode;
	}
	void setSensorMode(SensorMode sm)
	{
		sensorMode = (uint8_t)sm;
	}

	float getTemperatureCorrection() const
	{
		return int8_t(tempCorrection) / 10.f;
	}

	void setTemperatureCorrection(float val)
	{
		if (val > 5.f)
			val = 5.f;
		if (val < -5.f)
			val = -5.f;

		tempCorrection = int8_t(val*10);
	}

	float getInternalHysteresis() const
	{
		return internalHysteresis / 10.f;
	}

	void setInternalHysteresis(float val) 
	{
		internalHysteresis = uint8_t(val * 10);
	}

	float getExternalHysteresis() const
	{
		return externalHysteresis / 10.f;
	}

	void setExternalHysteresis(float val)
	{
		externalHysteresis = uint8_t(val * 10);
	}

	uint8_t getExternalSensorLimit() const
	{
		return externalSensorLimit;
	}

	void setExternalSensorLimit(uint8_t limit)
	{
		if (limit < 40)
			limit = 40;
		if (limit > 80)
			limit = 80;

		externalSensorLimit = limit;
	}

	virtual String toString()
	{
		return String("Settings 2: ") +
		    String("backlight mode: ") + String(getBacklightMode()) + 
		    String(", power mode: ") + String(getPowerMode()) + 
		    String(", antifreeze mode: ") + String(getAntifreezeMode()) +
		    String(", sensor mode: ") + String(getSensorMode()) +
		    String(", temp correct: ") + String(getTemperatureCorrection()) +
		    String(", int hyst: ") + String(getInternalHysteresis()) +
		    String(", ext hist: ") + String(getExternalHysteresis()) +
		    String(", ext limit: ") + String(getExternalSensorLimit())
		;
	}
};


class C17GH3MessageSchedule : public C17GH3MessageBase
{
public:
	C17GH3MessageSchedule(){}
	C17GH3MessageSchedule(int day) // day = 0 - 6
	{
		type = (uint8_t)MSG_TYPE_SCHEDULE_DAY1 + day;
	}
	uint8_t &time1         = bytes[3];  // 60 = 6am, 61 = 6:10am
	uint8_t &temperature1  = bytes[4];  // 10 = 5 degrees
	uint8_t &time2         = bytes[5];  // 60 = 6am
	uint8_t &temperature2  = bytes[6];  // 10 = 5 degrees
	uint8_t &time3         = bytes[7];  // 60 = 6am
	uint8_t &temperature3  = bytes[8];  // 10 = 5 degrees
	uint8_t &time4         = bytes[9];  // 60 = 6am
	uint8_t &temperature4  = bytes[10];  // 10 = 5 degrees
	uint8_t &time5         = bytes[11];  // 60 = 6am
	uint8_t &temperature5  = bytes[12];  // 10 = 5 degrees
	uint8_t &time6         = bytes[13];  // 60 = 6am
	uint8_t &temperature6  = bytes[14];  // 10 = 5 degrees

	uint8_t getHour(uint8_t idx) const
	{
		if (idx > 5)
			idx = 5;
		return bytes[3 + idx * 2] / 10;
	}
	void setTime(uint8_t idx, uint8_t hour, uint8_t minute)
	{
		if (hour > 23)
			hour = 23;
		if (minute > 59)
			minute = 59;
		if (idx > 5)
			idx = 5;
		bytes[3 + idx * 2] = hour * 10 +  minute/10;
	}

	uint8_t getMinute(uint8_t idx) const
	{
		if (idx > 5)
			idx = 5;
		
		return (bytes[3 + idx * 2] - bytes[3 + idx * 2]/10 * 10)*10;
	}

	uint8_t getTemperature(uint8_t idx) const
	{
		if (idx > 5)
			idx = 5;
		return bytes[4 + idx * 2] / 2.f;
	}
	void setTemperature(uint8_t idx, float temp)
	{
		if (idx > 5)
			idx = 5;
		bytes[4 + idx * 2] = (uint8_t)(temp * 2 + .5f);
	}


	virtual String toString()
	{
		String times;
		for (int i = 0 ; i < 6; ++i)
		{
			if (0 != i)
				times += ", ";
			times += String("time") + String(i+1) + String(": ") + String(getHour(i)) + String(":") + String(getMinute(i)) + 
		    	String(", temp") + String(i+1) + String(": ") + String(getTemperature(0));
		}

		return String("Schedule ") +
			String (type-0xC3 + 1) + String(": ") + times;
		;
	}
};


class C17GH3MessageBuffer
{
public:
	bool addbyte(uint8_t byte)
	{
		uint32_t newLastMS = millis();
		if (newLastMS - lastMS > 5)
			reset();

		lastMS = newLastMS;

		if (0 == currentByte)
		{
			if (0xAA == byte)
			{
				bytes[currentByte] = byte;
				++currentByte;
			}
				
		}
		else if (1 == currentByte)
		{
			if (0x55 == byte)
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
			if (15 == currentByte)
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

	void reset()
	{
		currentByte = 0;
	}
private:
	uint8_t bytes[16] = {0};
	uint8_t currentByte = 0;
	uint32_t lastMS = 0;
};


class C17GH3State
{
public:

	C17GH3MessageSettings1::WiFiState getWiFiState() const;
	bool getIsHeating() const;
	void setIsHeating(bool heating);
	bool getLock() const;
	void setLock(bool locked); 
	bool getMode() const;
	void setMode(bool mode_); 
	bool getPower() const;
	void setPower(bool pow) ;
	float getSetPointTemp() const;
	void setSetPointTemp(float temperature);
	float getInternalTemperature() const;
	float getExternalTemperature() const;

	bool getBacklightMode() const;
	void setBacklightMode(bool bl);
	bool getPowerMode() const;
	void setPowerMode(bool pm);
	bool getAntifreezeMode() const;
	void setAntifreezeMode(bool am);
	C17GH3MessageSettings2::SensorMode getSensorMode() const;
	void setSensorMode(C17GH3MessageSettings2::SensorMode sm);





	//C17GH3State::C17GH3State() {}
	void processRx();
	void processRx(int byte);
	void processRx(const C17GH3MessageBase& msg);
	void processTx(bool timeAvailable = false) const;
	void sendMessage(const C17GH3MessageBase& msg) const;


	typedef std::function<void()> WifiConfigCallback;
	void setWifiConfigCallback(WifiConfigCallback cb)
	{
		wifiConfigCallback = cb;
	}

	String toString()
	{
		String str;
		str += "HEATING: ";
		if (getIsHeating())
			str  += "ON\n";
		else
			str += "OFF\n";
		if (settings1.isValid())
		{
			str += settings1.toString();
			str += "\n";
		}
		
		if (settings2.isValid())
		{
			str += settings2.toString();
			str += "\n";
		}
		for (int i = 0; i < 7; ++i)
		{
			if (schedule[i].isValid())
			{
				str += schedule[i].toString();
				str += "\n";
			}
		}
		return str;
	}
private:
	bool isValidState(const C17GH3MessageBase::C17GH3MessageType &msgType) const;
	void sendSettings1(bool timeAvailable = false) const;
	void sendSettings2() const;

	C17GH3MessageBuffer msgBuffer;

	C17GH3MessageSettings1 settings1;
	C17GH3MessageSettings2 settings2;
	C17GH3MessageSchedule schedule[7];

	WifiConfigCallback wifiConfigCallback;
	mutable bool firstQueriesDone = false;
	bool isHeating = false;

};

#endif