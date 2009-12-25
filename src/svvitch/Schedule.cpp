#include "Schedule.h"

#include <Poco/format.h>


Schedule::Schedule(const string id, const int year, const int month, const int day, const int hour, const int minute, const int second, const int week, const string command):
	_log(Poco::Logger::get("")),
	_id(id), _year(year), _month(month), _day(day), _hour(hour), _minute(minute), _second(second), _week(week), _command(command)
{
	string time = Poco::format("%d/%d/%d %d:%d:%d", _year, _month, _day, _hour, _minute, _second);
	_log.information(Poco::format("schedule %s(%d)->%s", time, _week, _command));
}

Schedule::~Schedule() {
}

bool Schedule::check(LocalDateTime time) {
	if (_year >= 0 && _year != time.year()) return false;
	if (_month >= 0 && _month != time.month()) return false;
	if (_week >= 0 && _week != time.dayOfWeek()) return false;
	if (_day >= 0 && _day != time.day()) return false;
	if (_hour >= 0 && _hour != time.hour()) return false;
	if (_minute >= 0 && _minute != time.minute()) return false;
	if (_second >= 0 && _second != time.second()) return false;
	return true;
}

const string& Schedule::command() const {
	return _command;
}
