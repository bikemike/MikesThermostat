#ifndef THERMOSTAT_STATE_H
#define THERMOSTAT_STATE_H

#include <Arduino.h>
#include <set>
#include <sys/time.h>

class MessageInterface
{
public:
	virtual ~MessageInterface() {}
	virtual const uint8_t* getBytes() const = 0;
	virtual uint32_t getLength() const = 0;
	virtual bool isValid() const = 0;
};


class ThermostatState
{
public:
	enum Mode
	{
		MODE_SCHEDULE,
		MODE_MANUAL,
	};
public:
	virtual ~ThermostatState() {}

	virtual void loop() = 0;


	void setTimeAvailable(bool available) { timeAvailable = available; }
	bool getTimeAvailable() const { return timeAvailable; }


	virtual void processRx() = 0;
	virtual void processRx(int byte) = 0;
	virtual void processTx(bool timeAvailable = false) = 0;
	virtual void sendMessage(const MessageInterface& msg) const = 0;


	typedef std::function<void()> WifiConfigCallback;
	void setWifiConfigCallback(WifiConfigCallback cb)
	{
		wifiConfigCallback = cb;
	}


	virtual bool getIsHeating() const { return isHeating; }

	virtual void setIsHeating(bool heating)
	{
		if (isHeating != heating)
		{
			isHeating = heating;
			emitChange(ChangeEvent::CHANGE_TYPE_IS_HEATING);
		}
	}

	virtual void setAuxHeatEnabled(bool on) { if (on != auxHeatEnable) { auxHeatEnable = on; emitChange(ChangeEvent::CHANGE_TYPE_AUX_HEAT_ENABLED); }}
	virtual bool getAuxHeatEnabled() const { return auxHeatEnable; }

	virtual void setPower(bool on, bool updateMCU = false)  = 0;
	virtual void setSetPointTemp(float temp, bool updateMCU = false)  = 0;
	virtual void setMode(Mode mode, bool updateMCU = false)  = 0;
	virtual void setEconomy(bool econ, bool updateMCU = false) {}
	virtual void setLock(bool lock, bool updateMCU = false)  = 0;

	virtual bool getPower() const  = 0;
	virtual float getSetPointTemp() const  = 0;
	virtual float getInternalTemp() const  = 0;
	virtual float getExternalTemp() const   = 0;
	virtual Mode getMode() const  = 0;
	virtual bool getEconomy() const  {return false;}
	virtual bool getLock() const  = 0;

	virtual void setSchedule(const uint8_t* schedule, uint8_t length, bool updateMCU = false)  = 0;

	virtual int getScheduleDay() const = 0;
	virtual int getSchedulePeriod() const = 0;
	virtual float getScheduleCurrentPeriodSetPointTemp() const = 0;
	virtual float getScheduleSetPointTemperature(int day, int period) const = 0;



	virtual String toString() = 0;

	class ChangeEvent
	{
	public:
		enum ChangeType
		{
			CHANGE_TYPE_NONE,
			CHANGE_TYPE_POWER,
			CHANGE_TYPE_AUX_HEAT_ENABLED,
			CHANGE_TYPE_MODE,
			CHANGE_TYPE_ECONOMY,
			CHANGE_TYPE_CURRENT_SCHEDULE_PERIOD,
			CHANGE_TYPE_SETPOINT_TEMP,
			CHANGE_TYPE_INTERNAL_TEMP,
			CHANGE_TYPE_EXTERNAL_TEMP,
			CHANGE_TYPE_IS_HEATING,
			CHANGE_TYPE_SCHEDULE,
			CHANGE_TYPE_LOCK,
		};
	public:
		ChangeEvent(ChangeType t) : type (t) {}
		ChangeType getType() const { return type; }

		bool operator==(const ChangeEvent& other) const
		{
			return type == other.type;
		}
		bool operator<(const ChangeEvent& other) const
		{
			return type < other.type;
		}
	private:
		ChangeType type = CHANGE_TYPE_NONE;
	};

	class Listener
	{
	public:
		virtual ~Listener(){}
		virtual void handleThermostatStateChange(const ChangeEvent& c) = 0;
	};

	void addListener(Listener* listener)
	{
		if (listener)
			listeners.insert(listener);
	}

protected:
	virtual void setInternalTemp(float temp)  = 0;
	virtual void setExternalTemp(float temp)  = 0;

	virtual void emitChange(const ChangeEvent& c)
	{
		for(auto l : listeners)
		{
			l->handleThermostatStateChange(c);
		}
	};


	bool getTime(int &dayOfWeek, int &hour, int &minutes)
	{
		bool gotTime = false;
		struct tm* new_time = nullptr;
		if (getTimeAvailable())
		{
			struct timezone tz = {0};
			timeval tv = {0};
			
			gettimeofday(&tv, &tz);
			time_t tnow = time(nullptr);

			new_time = localtime(&tnow);
			// sunday = 0, sat = 6
			dayOfWeek = new_time->tm_wday;
			hour = new_time->tm_hour;
			minutes = new_time->tm_min;
			
			gotTime = true;
		}
		return gotTime;
	}

private:
	std::set<Listener*> listeners;
	WifiConfigCallback wifiConfigCallback;
	bool isHeating = false;
	bool auxHeatEnable   = false;
	bool timeAvailable = false;
};


#endif
