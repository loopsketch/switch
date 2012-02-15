#pragma once

#include <Poco/Channel.h>
#include <Poco/Message.h>


class DebugConsoleChannel: public Poco::Channel {
private:

public:
	DebugConsoleChannel();

	void log(const Poco::Message& msg);
		
protected:
	~DebugConsoleChannel();
};
