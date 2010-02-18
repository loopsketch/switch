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

	/** 日付系の指定がマッチしているか */
	bool Schedule::matchDate(LocalDateTime time);

	/** 実行タイミングにjustマッチしているか */
	bool match(LocalDateTime time);

	/** 日付系がマッチしており実行タイミングを経過しているか */
	bool matchPast(LocalDateTime time);

	/** コマンド */
	const string& command() const;
};

typedef Schedule* SchedulePtr;