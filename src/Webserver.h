#ifndef WEBSERVER_H
#define WEBSERVER_H



class Webserver
{
public:
	Webserver() {}
	void init(class C17GH3State* state);
	void stop();
	void start();

	void handleRoot();
	void handleNotFound();
	void process();
private:
	class ESP8266WebServer* server = nullptr;
	class C17GH3State* state = nullptr;
	class ESP8266HTTPUpdateServer* httpUpdater = nullptr;

};
#endif