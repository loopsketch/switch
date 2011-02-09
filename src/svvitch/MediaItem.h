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
	MediaTypeMix,
	MediaTypeMovie,
	MediaTypeImage,
	MediaTypeText,
	MediaTypeFlash,
	MediaTypeBrowser,
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
	string _params;
	Poco::HashMap<string, string> _properties;

	MediaItemFile& copy(const MediaItemFile& mif) {
		if (this == &mif) return *this;
		_type = mif._type;
		_file = mif._file;
		_params = mif._params;
		_properties = mif._properties;
		return *this;
	}

public:
	MediaItemFile(const MediaType type, const string file, const string params): _log(Poco::Logger::get("")),
		_type(type), _file(file), _params(params)
	{
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
		copy(mif);
	}

	MediaItemFile& operator=(const MediaItemFile& mif) {
		return copy(mif);
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

	const string& params() const {
		return _params;
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
	const string NULL_STRING;
	Poco::Logger& _log;

	MediaType _type;
	string _id;
	string _name;
	int _start;
	int _duration;
	string _params;
	Poco::HashMap<string, string> _properties;
	vector<MediaItemFile> _files;

	MediaItem& copy(const MediaItem& item) {
		if (this == &item) return *this;
		_type = item._type;
		_id = item._id;
		_name = item._name;
		_start = item._start;
		_duration = item._duration;
		_params = item._params;
		_properties = item._properties;
		_files = item._files;
		return *this;
	}

public:
	MediaItem(const MediaType type, const string id, const string name, const int start, const int duration, const string params, const vector<MediaItemFile> files):
		_log(Poco::Logger::get("")), _type(type), _id(id), _name(name), _start(start), _duration(duration), _params(params), _files(files)
	{
		if (!params.empty()) {
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

	MediaItem(const MediaItem& item): _log(Poco::Logger::get("")) {
		copy(item);
	}

	MediaItem& operator=(const MediaItem& mif) {
		return copy(mif);
    }

	virtual ~MediaItem(void) {
//		for (vector<MediaItemFile>::iterator it = _files.begin(); it != _files.end(); it++) SAFE_DELETE(*it);
		_files.clear();
		_properties.clear();
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

	const string& params() const {
		return _params;
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

	const int fileCount() {
		return _files.size();
	}

	const vector<MediaItemFile>& files() {
		return _files;
	}
};


typedef MediaItem* MediaItemPtr;
typedef MediaItemFile* MediaItemFilePtr;
