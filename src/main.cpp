#include <Arduino.h>

#include <ESP8266WiFi.h> 
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FS.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

// time includes
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>                  // settimeofday_cb()

#include <set>

#include "C17GH3.h"
#include "Webserver.h"
#include "Log.h"

WiFiManager wifiManager;
C17GH3State state;
Webserver webserver;
Log logger;

bool inSerial = false;
uint32_t lastMS = 0;

#define TIMEZONE 	"PST8PDT,M3.2.0,M11.1.0" // FROM https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json

static void setupOTA();
static void initTime();

void setup()
{
	String devName = String("Thermostat-") + String(ESP.getChipId(),HEX);
	ArduinoOTA.setHostname(devName.c_str());
	WiFi.hostname(devName);
	WiFi.enableAP(false);
	WiFi.enableSTA(true);
	WiFi.begin();
	wifiManager.setConfigPortalTimeout(120);
	wifiManager.setDebugOutput(false);

	Serial.begin(9600);

	state.setWifiConfigCallback([devName]() {
		logger.addLine("Configuration portal opened");
		webserver.stop();
        wifiManager.startConfigPortal(devName.c_str());
		webserver.start();
		WiFi.enableAP(false);
		logger.addLine("Configuration portal closed");
    });

	webserver.init(&state);
	setupOTA();

	MDNS.begin(devName);
	MDNS.addService("http", "tcp", 80);

	initTime();

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
	state.processRx();
	webserver.process();
	ArduinoOTA.handle();
	state.processTx(cbtime_set > 1);
	MDNS.update();

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

/*

#define  "192.168.31.107" // Enter your MQTT server adderss or IP. I use my DuckDNS adddress (yourname.duckdns.org) in this field
#define mqtt_user "DVES_USER" //enter your MQTT username
#define mqtt_password "DVES_PASS" //enter your password

#define topic_base "livingroom/wifiremote/"
#define send_raw topic_base"send/raw"
#define send_samsung topic_base"send/samsung"
#define send_rc5 topic_base"send/rc5"
#define send_rc6 topic_base"send/rc6"
#define send_nec topic_base"send/nec"
#define send_sony topic_base"send/sony"
#define send_jvc topic_base"send/jvc"
#define send_whynter topic_base"send/whynter"
#define send_aiwarct501 topic_base"send/aiwar"
#define send_lg topic_base"send/lg"
#define send_dish topic_base"send/dish"
#define send_sharp topic_base"send/sharp"
#define send_sharpraw topic_base"send/sharpraw"
#define send_denon topic_base"send/denon"
#define send_pronto topic_base"send/pronto"
#define send_legopower topic_base"send/legopower"

WiFiClient espClient;
PubSubClient mqttClient(espClient); //this needs to be unique for each controller


void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	char buffer[64] = {0};

	if (length >= 64)
		return; // message too big

	memcpy(buffer, payload, length);

	// sendsamsung: data=aaeeff00,bits=32,repeat=5
	// raw data=aa00ff,hz=300
	// ...
	uint32_t bits = 0;
	uint32_t repeat = 0;
	//uint32_t data[16] = {0};
	uint64_t data64 = 0;

	char *saveptr1 = NULL, *saveptr2 = NULL;

	char* token1 = strtok_r(buffer, ",", &saveptr1); 

	while (token1 != NULL)
	{
		char* name = strtok_r(token1,"=",&saveptr2);
		if (name != NULL)
		{
			char* value = strtok_r(NULL,"=",&saveptr2);
			if (NULL != value)
			{
				Serial.print(name);
				Serial.print(" = ");
				Serial.print(value);
				Serial.println();
				if (String(name) == "data")
				{
						// convert hex string
						char* endptr = NULL;
						uint32_t val = strtoul (value, &endptr, 16);
						Serial.print("value in hex: ");
						Serial.println(val, HEX);
						data64 = val;
						

				}
				else if (String(name) == "bits")
				{
					// convert int string to int
					bits = atoi(value);
					
				}
				else if (String(name) == "repeat")
				{
					repeat = atoi(value);
				}

			}
			token1 = strtok_r(NULL,",",&saveptr1);
		}
	} 

	if (String(topic) == send_samsung)
	{
		if (0 != data64 && bits == 32 && repeat <= 20)
		{
			Serial.print("Sending data to samsung data=");
			Serial.print((uint32_t)data64,HEX);
			Serial.print(", bits=");
			Serial.print(bits);
			Serial.print(", repeat=");
			Serial.println(repeat);
			irsend.sendSAMSUNG(data64, bits, repeat);
		}
		else
		{
			Serial.print("Data Error: Not sending data to samsung data=");
			Serial.print((uint32_t)data64,HEX);
			Serial.print(", bits=");
			Serial.print(bits);
			Serial.print(", repeat=");
			Serial.println(repeat);
		}
		
	}
}

uint32_t mqttNextConnectAttempt = 0;

void mqttReconnect()
{

	uint32_t now = millis();
	if (now > mqttNextConnectAttempt)
	{
		// Loop until we're reconnected
		if (!mqttClient.connected())
		{
			Serial.print("Attempting MQTT connection...");
			// Create a random client ID
			String clientId = "devname-";
			clientId += String(random(0xffff), HEX);
			// Attempt to connect
			if (mqttClient.connect(devname, mqtt_user, mqtt_password))
			{
				Serial.println("connected");
				// Once connected, publish an announcement...
				//mqttClient.publish("outTopic", "hello world");
				// ... and resubscribe
				mqttClient.subscribe(send_samsung);
				mqttClient.subscribe(send_raw);
				mqttClient.subscribe(send_rc5);
				mqttClient.subscribe(send_rc6);
				mqttClient.subscribe(send_nec);
				mqttClient.subscribe(send_sony);
				mqttClient.subscribe(send_jvc);
				mqttClient.subscribe(send_whynter);
				mqttClient.subscribe(send_aiwarct501);
				mqttClient.subscribe(send_lg);
				mqttClient.subscribe(send_dish);
				mqttClient.subscribe(send_sharp);
				mqttClient.subscribe(send_sharpraw);
				mqttClient.subscribe(send_denon);
				mqttClient.subscribe(send_pronto);
				mqttClient.subscribe(send_legopower);
				mqttClient.subscribe(send_samsung);
			}
			else
			{
				Serial.print("failed, rc=");
				Serial.print(mqttClient.state());
				Serial.println(" try again in 5 seconds");
				// Wait 5 seconds before retrying
				//delay(5000);
				mqttNextConnectAttempt = now + 2000;
			}
		}
	}
}

*/