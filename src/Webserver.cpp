#include "Webserver.h"
//#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "C17GH3.h"


void Webserver::init(C17GH3State* state_)
{
	state = state_;
	server = new ESP8266WebServer(80);
	httpUpdater = new ESP8266HTTPUpdateServer();

	server->on("/", HTTP_GET,std::bind(&Webserver::handleRoot, this));
	server->on("console", HTTP_GET,std::bind(&Webserver::handleConsole, this));
	server->onNotFound(std::bind(&Webserver::handleRoot, this));
	server->begin();

	httpUpdater->setup(server);
}

void Webserver::handleRoot()
{

	String msg("<html><head><title>Wifi Thermostat</title></head><body>");
	msg += "State:<br>";
	msg += "<pre>";
	msg += state->toString();
	msg += "</pre>";
	
	msg += "<a href=\"/update\">Update firmware</a>";
	msg += String("</body></html>");
	server->send(200, "text/html", msg);
}

void Webserver::handleNotFound()
{
	//if (!handleFileRead(server->uri()))
		server->send(404, "text/plain", "Not Found.");
}

void Webserver::handleConsole()
{
	String msg("<html><head><title>Wifi Thermostat</title></head><body>");
	msg += "State:<br>";
	msg += "<pre>";
	msg += "<form method='POST'>";
	msg += "<textarea style='width:600px;height:300px'>";
	msg += debug.buffer();
	msg += "</textarea>";
	msg += "<br/>";
	msg += "<input type='text' style='width:500px;'></input><input type=button value='send' style='width:100px;'></input>";
	msg += "</form>";

	msg += "</body></html>";
	server->send(200, "text/html", msg);
}

void Webserver::start()
{
	server->begin();
}

void Webserver::stop()
{
	server->stop();
}


void Webserver::process()
{
	server->handleClient();
}