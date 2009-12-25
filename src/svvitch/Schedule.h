#pragma once

#include <Poco/Logger.h>
#include <Poco/LocalDateTime.h>

using std::string;
using Poco::LocalDateTime;


class Schedule {
private:
	Poco::Logger& _log;

	string _id;
	int _year;
	int _month;
	int _day;
	int _hour;
	int _minute;
	int _second;
	int _week;
	string _command;

public:
	Schedule(const string id, const int year, const int month, const int day, const int hour, const int minute, const int second, const int week, const string command);
	~Schedule();

	/** 実行タイミングかどうか */
	bool check(LocalDateTime time);

	/** コマンド */
	const string& command() const;
};

typedef Schedule* SchedulePtr;