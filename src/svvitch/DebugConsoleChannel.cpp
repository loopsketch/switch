#ifdef _DEBUG

#include "DebugConsoleChannel.h"

#include <Poco/UnicodeConverter.h>


DebugConsoleChannel::DebugConsoleChannel() {
}

DebugConsoleChannel::~DebugConsoleChannel() {
}

void DebugConsoleChannel::log(const Poco::Message& msg) {
	std::wstring ws;
	Poco::UnicodeConverter::toUTF16(msg.getText(), ws);
	ws.append(L"\n");
	::OutputDebugStringW(ws.c_str());
}

#endif
