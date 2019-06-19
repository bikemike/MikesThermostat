#include "Webserver.h"
//#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "C17GH3.h"
#include "Log.h"

extern Log logger;

void Webserver::init(C17GH3State* state_)
{
	state = state_;
	server = new ESP8266WebServer(80);
	httpUpdater = new ESP8266HTTPUpdateServer();

	server->on("/", HTTP_GET,std::bind(&Webserver::handleRoot, this));
	server->on("/console", HTTP_GET,std::bind(&Webserver::handleConsole, this));
	server->on("/console", HTTP_POST,std::bind(&Webserver::handleConsole, this));
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
	if (server->hasArg("cmd"))
    {
		String cmd = server->arg("cmd");
		cmd.trim();
		cmd.replace(" ","");
		logger.addLine("Got a post cmd: " + cmd);
		
		if (cmd.length() == 32)
		{
			while (cmd.length() > 0)
			{
				String v = cmd.substring(0,2);
				cmd = cmd.substring(2);
				state->processRx(strtol(v.c_str(), nullptr, 16));
			}
		}
    }
	String msg("<html><head><title>Wifi Thermostat</title>");
	msg += "<script>";
	msg += "function sb(){ var c = document.getElementById('console');c.scrollTop = c.scrollHeight;}";
	msg += "</script></head>";
	msg += "<body onload='sb();'>";
	msg += "Console:<br>";
	msg += "<form method='POST'>";
	msg += "<textarea id='console' style='width:100%;height:calc(100% - 50px);'>";
	msg += logger.getLines();
	msg += "</textarea>";
	msg += "<br/>";
	msg += "<input type='text' name='cmd' style='width:calc(100% - 100px);'></input><input type=submit value='send' style='width:100px;'></input>";
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