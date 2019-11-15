#include <Arduino.h>

#include <ESP8266WiFi.h> 
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FS.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>

// time includes
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>                  // settimeofday_cb()

#include <set>

#ifdef C17GH3
#include "C17GH3.h"
#endif
#ifdef BHT002GBLW
#include "BHT002.h"
#endif
#include "Webserver.h"
#include "Log.h"
#include "ThermostatMQTT.h"

WiFiManager wifiManager;
#ifdef C17GH3
C17GH3State state;
#else
TUYAThermostatState state;
#endif
ThermostatMQTT thermostatMQTT(&state);
Webserver webserver;
Log logger;
bool relayOn = false;

//#define PIN_RELAY_MONITOR    5

#ifdef C17GH3
//#define PIN_RELAY2           4
//#define PIN_PWM             13	
//#define PIN_PWM_MONITOR     12
#endif


class AuxHeatHandler : public ThermostatState::Listener
{
public:
	AuxHeatHandler(ThermostatState* state) : state(state)
	{																																																																																																																																																																																																																																																																																																																																																																																																																																										
		state->addListener(this);

	}

	virtual void handleThermostatStateChange(const ThermostatState::ChangeEvent& c) override
	{
		switch(c.getType())
		{
		case ThermostatState::ChangeEvent::CHANGE_TYPE_AUX_HEAT_ENABLED:
			{
#ifdef PIN_RELAY2
				if (state && state->getAuxHeatEnabled() && state->getIsHeating())
				{
					digitalWrite(PIN_RELAY2, HIGH);
				}
				else
				{
					digitalWrite(PIN_RELAY2, LOW);					
				}
				
#endif
			}
			break;
		default:
			break;
		}
	}
private:
	ThermostatState* state = nullptr;

};

AuxHeatHandler auxHeatHandler(&state);

#define TIMEZONE 	"PST8PDT,M3.2.0,M11.1.0" // FROM https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json

static void setupOTA();
static void initTime();


#ifdef PIN_RELAY_MONITOR
static void ICACHE_RAM_ATTR handleRelayMonitorInterrupt();
#endif

#ifdef PIN_PWM_MONITOR

static uint32_t pwmLowCycles = 0;
static uint32_t pwmHighCycles = 0;
static int      pwmPinValue = LOW;
static uint32_t pwmLastInterruptCycles = 0;
static const uint32_t pwmMaxPeriod = 11000000;
static bool pwmInterruptTimedOut = false;
static void ICACHE_RAM_ATTR handlePWMMonitorInterrupt();
#endif

struct Config
{
	char deviceName[64] = {0};
};

Config config;


static void saveConfig()
{
	const char* filename = "/config.json";
	File file = SPIFFS.open(filename, "w");
	bool saved = false;
	if (file)
	{
		StaticJsonDocument<256> doc;
		doc["deviceName"] = config.deviceName;
		size_t bytes_written = serializeJson(doc, file);
		saved = (0 != bytes_written);
		file.close();

	}
	if (!saved)
		logger.addLine("ERROR: unable to save /config.json");
}

static void loadConfig()
{
	String devName = String("Thermostat-") + String(ESP.getChipId(),HEX);
	const char* filename = "/config.json";
	bool loaded = false;

	if (SPIFFS.exists(filename))
	{
		File file = SPIFFS.open(filename, "r");

		StaticJsonDocument<512> doc;

		DeserializationError error = deserializeJson(doc, file);
		if (DeserializationError::Ok == error)
		{
			strlcpy(
				config.deviceName,
				doc["deviceName"] | devName.c_str(),
				sizeof(config.deviceName));
			file.close();
			loaded = true;
		}
		
	}
	if (!loaded)
	{
		logger.addLine("ERROR: unable to load /config.json");

		strlcpy(
			config.deviceName,
			devName.c_str(),
			sizeof(config.deviceName));

		saveConfig();
	}
}


void setup()
{
	SPIFFS.begin();
	loadConfig();
	//strlcpy(config.deviceName, "MoesThermostat",16);
	//saveConfig();

	thermostatMQTT.setName(config.deviceName);
	ArduinoOTA.setHostname(config.deviceName);
	WiFi.hostname(config.deviceName);
	WiFi.enableAP(false);
	WiFi.enableSTA(true);
	WiFi.begin();
	wifiManager.setConfigPortalTimeout(120);
	wifiManager.setDebugOutput(false);

	Serial.begin(9600);

	state.setWifiConfigCallback([]() {
		logger.addLine("Configuration portal opened");
		webserver.stop();
        wifiManager.startConfigPortal(config.deviceName);
		webserver.start();
		WiFi.enableAP(false);
		logger.addLine("Configuration portal closed");
    });

	//wifiManager.autoConnect(devName.c_str());

	webserver.init(&state, config.deviceName);
	setupOTA();

	MDNS.begin(config.deviceName);
	MDNS.addService("http", "tcp", 80);

	initTime();
#ifdef PIN_RELAY_MONITOR
	pinMode(PIN_RELAY_MONITOR, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_RELAY_MONITOR), handleRelayMonitorInterrupt, CHANGE);
	relayOn = digitalRead(PIN_RELAY_MONITOR);
	state.setIsHeating(relayOn);
#endif
#ifdef PIN_RELAY2
	pinMode(PIN_RELAY2, OUTPUT);
	digitalWrite(PIN_RELAY2, LOW); 	
#endif

#ifdef PIN_PWM
	pinMode(PIN_PWM, OUTPUT);
	analogWrite(PIN_PWM, 1023);
#endif

#ifdef PIN_PWM_MONITOR
	pinMode(PIN_PWM_MONITOR, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_PWM_MONITOR), handlePWMMonitorInterrupt, CHANGE);
#endif
}

timeval cbtime;			// when time set callback was called
int cbtime_set = 0;
static void timeSet()
{
	gettimeofday(&cbtime, NULL);
	cbtime_set++;
}



void loop()
{
	if (relayOn != state.getIsHeating())
	{
		state.setIsHeating(relayOn);
#ifdef PIN_RELAY2
		if (state.getAuxHeatEnabled())
		{
			digitalWrite(PIN_RELAY2, relayOn ? HIGH : LOW);
		}
#endif
	}
	
#if defined(PIN_PWM) && defined (PIN_PWM_MONITOR)
	float fpwm = (float)pwmHighCycles / (pwmHighCycles + pwmLowCycles); // pwm range is about 25-100
	if (fpwm < .27f)
		fpwm = .27f; // so it doesn't jump around .24 to .27 when screen idle
	
	//int pwm = int(100 * ((fpwm - .26) * .74) + .5);
	int pwm = int(100 * fpwm);
	uint32_t cycles = ESP.getCycleCount();
	if (pwmLastInterruptCycles - cycles < 10000)
		cycles = pwmLastInterruptCycles; // probably interrupted when calling ESP.getCycleCount
	if (pwmInterruptTimedOut ||  pwmMaxPeriod <  cycles - pwmLastInterruptCycles)
	{
		logger.addLine(String(cycles) + String(" - ") + String(pwmLastInterruptCycles) + String(" = ") + String(cycles - pwmLastInterruptCycles) );
		pwmInterruptTimedOut = true;
		if (HIGH == pwmPinValue)
			pwm = 100;
		else
			pwm = 0;
	}

	if ( state.getPWM() != pwm )
	{
		logger.addLine(String("PWM ") + String(pwm) + " !+ " + String(state.getPWM()));
	
		//analogWrite(PIN_PWM, pwm == 100 ? 1023 : 0);
		analogWrite(PIN_PWM, int(1023 * (pwm*0.01f)));
		// pwm night mode (screen off at night)
		// idle brightness 0-100%
		// active_brightness
	}
	state.setPWM(pwm);
#endif
	state.setTimeAvailable(cbtime_set > 1);
	state.loop();

	webserver.process();
	ArduinoOTA.handle();
	MDNS.update();
	thermostatMQTT.loop();
}

static void setupOTA()
{
	ArduinoOTA.onStart([]() 
	{
	});
	ArduinoOTA.onEnd([]()
	{
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
	{
	});
	
	ArduinoOTA.onError([](ota_error_t error)
	{
		/*
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
		*/
	});
	ArduinoOTA.begin();
}



static void initTime()
{

	// set function to call when time is set
	// is called by NTP code when NTP is used
	settimeofday_cb(timeSet);

	// set time from RTC
	// Normally you would read the RTC to eventually get a current UTC time_t
	// this is faked for now.
	time_t rtc_time_t = 1541267183; // fake RTC time for now

	timezone tz = { 0, 0};
	timeval tv = { rtc_time_t, 0};

	// DO NOT attempt to use the timezone offsets
	// The timezone offset code is really broken.
	// if used, then localtime() and gmtime() won't work correctly.
	// always set the timezone offsets to zero and use a proper TZ string
	// to get timezone and DST support.

	// set the time of day and explicitly set the timezone offsets to zero
	// as there appears to be a default offset compiled in that needs to
	// be set to zero to disable it.
	settimeofday(&tv, &tz);


	// set up TZ string to use a POSIX/gnu TZ string for local timezone
	// TZ string information:
	// https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
	setenv("TZ", TIMEZONE, 1);

	tzset(); // save the TZ variable

	// enable NTP by setting up NTP server(s)
	// up to 3 ntp servers can be specified
	// configTime(tzoffset, dstflg, "ntp-server1", "ntp-server2", "ntp-server3");
	// set both timezone offet and dst parameters to zero 
	// and get real timezone & DST support by using a TZ string
	configTime(0, 0, "pool.ntp.org");
}

#ifdef PIN_RELAY_MONITOR

static void ICACHE_RAM_ATTR handleRelayMonitorInterrupt()
{
	relayOn = digitalRead(PIN_RELAY_MONITOR);
}
#endif
#ifdef PIN_PWM_MONITOR

static void ICACHE_RAM_ATTR handlePWMMonitorInterrupt()
{
	uint32_t current = ESP.getCycleCount();
	uint32_t duration = current - pwmLastInterruptCycles;
	pwmLastInterruptCycles = current;

	pwmPinValue = digitalRead(PIN_PWM_MONITOR);
	if (HIGH == pwmPinValue)
	{
		pwmLowCycles = duration;
	}
	else
	{
		pwmHighCycles = duration;
	}
	pwmInterruptTimedOut = false;
}
#endif
