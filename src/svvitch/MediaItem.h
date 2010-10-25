#pragma once

#include <Poco/Logger.h>
#include <vector>
#include <Poco/File.h>
#include <Poco/format.h>
#include <Poco/NumberParser.h>
#include <Poco/HashMap.h>
#include <Poco/String.h>
#include <Poco/Stringtokenizer.h>

#include "Common.h"

using Poco::File;
using std::string;
using std::vector;


enum MediaType {
	MediaTypeMovie,
	MediaTypeImage,
	MediaTypeText,
	MediaTypeFlash,
	MediaTypeCv,
	MediaTypeCvCap,
	MediaTypeGame,
	MediaTypeGame2,
	MediaTypeUnknown,
};


class MediaItemFile
{
private:
	const string NULL_STRING;
	Poco::Logger& _log;
	MediaType _type;
	string _file;
	Poco::HashMap<string, string> _properties;

public:
	MediaItemFile(const MediaType type, const string file, const string params): _log(Poco::Logger::get("")), _type(type), _file(file) {
		if (!params.empty()) {
//			_log.information(Poco::format("properties: %s", file));
//			_file = file.substr(0, file.find("?"));
//			string params = file.substr(file.find("?") + 1);
			Poco::StringTokenizer props(params, ",");
			for (Poco::StringTokenizer::Iterator it = props.begin(); it != props.end(); it++) {
				string pair = *it;
				int i = pair.find("=");
				if (i != string::npos) {
					string key = Poco::trim(pair.substr(0, i));
					string value = Poco::trim(pair.substr(i + 1));
					_properties[key] = value;
//					_log.information(Poco::format("property[%s]=<%s>", key, value));
				}
			}
		}
	}

	MediaItemFile(const MediaItemFile& mif): _log(Poco::Logger::get("")) {
		_type = mif._type;
		_file = mif._file;
		_properties = mif._properties;
	}

	MediaItemFile& operator=(const MediaItemFile& mif) {
		if (this == &mif) return *this;
		_type = mif._type;
		_file = mif._file;
		_properties = mif._properties;
		return *this;
    }

	virtual ~MediaItemFile() {
		// _properties.clear();
	}

	const MediaType type() const {
		return _type;
	}

	const string& file() const {
		return _file;
	}

	const string& getProperty(const string& key) const {
		if (_properties.find(key) != _properties.end()) {
			return _properties[key];
		}
		return NULL_STRING;
	}

	const string& getProperty(const string& key, const string& defaultValue) const {
		if (_properties.find(key) != _properties.end()) {
			return _properties[key];
		}
		return defaultValue;
	}

	const int getNumProperty(const string& key, const int& defaultValue) const {
		int num = defaultValue;
		Poco::NumberParser::tryParse(getProperty(key), num);
		return num;
	}

	const DWORD getHexProperty(const string& key, const DWORD& defaultValue) const {
		Poco::UInt64 num = defaultValue;
		Poco::NumberParser::tryParseHex64(getProperty(key), num);
		return (DWORD)num;
	}

	const double getFloatProperty(const string& key, const double& defaultValue) const {
		double num = defaultValue;
		Poco::NumberParser::tryParseFloat(getProperty(key), num);
		return num;
	}
};


class MediaItem
{
private:
	Poco::Logger& _log;

	MediaType _type;
	string _id;
	string _name;
	int _start;
	int _duration;
	bool _stanby;
	vector<MediaItemFile> _files;

public:
	MediaItem(const MediaType type, const string id, const string name, const int start, const int duration, const bool stanby, const vector<MediaItemFile> files):
		_log(Poco::Logger::get("")), _type(type), _id(id), _name(name), _start(start), _duration(duration), _stanby(stanby), _files(files) {

	}

	MediaItem(const MediaItem& item): _log(Poco::Logger::get("")) {
		_type = item._type;
		_id = item._id;
		_name = item._name;
		_start = item._start;
		_duration = item._duration;
		_files = item._files;
	}

	virtual ~MediaItem(void) {
//		for (vector<MediaItemFile>::iterator it = _files.begin(); it != _files.end(); it++) SAFE_DELETE(*it);
		_files.clear();
	}

	const MediaType type() const {
		return _type;
	}

	const bool containsFileType(const MediaType type) {
		for (vector<MediaItemFile>::iterator it = _files.begin(); it != _files.end(); it++) {
			if ((*it).type() == type) return true;
		}
		return false;
	}

	const string& id() const {
		return _id;
	}

	const string& name() const {
		return _name;
	}

	const int start() const {
		return _start;
	}

	const int duration() const {
		return _duration;
	}

	const bool stanby() const {
		return _stanby;
	}

	const int fileCount() {
		return _files.size();
	}

	const vector<MediaItemFile>& files() {
		return _files;
	}
};


typedef MediaItem* MediaItemPtr;
