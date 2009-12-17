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
	MediaTypeCv,
	MediaTypeCvCap
};


class MediaItemFile
{
private:
	string NULL_STRING;
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

	virtual ~MediaItemFile() {
		_properties.clear();
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
//		try {
//			return _properties[key];
//		} catch (Poco::NotFoundException ex) {
//			_log.warning(Poco::format("property not found: %s", key));
//		}
		return NULL_STRING;
	}

	const int getNumProperty(const string& key, const int& defaultValue) const {
		int num = defaultValue;
		Poco::NumberParser::tryParse(getProperty(key), num);
		return num;
	}

	const double getFloatProperty(const string& key, const double& defaultValue) const {
		double num = defaultValue;
		Poco::NumberParser::tryParseFloat(getProperty(key), num);
		return num;
	}
};

typedef MediaItemFile* MediaItemFilePtr;


class MediaItem
{
private:
	Poco::Logger& _log;

	MediaType _type;
	string _id;
	string _name;
	int _duration;
	vector<MediaItemFilePtr> _files;

public:
	MediaItem(const MediaType type, const string id, const string name, const int duration, const vector<MediaItemFilePtr> files):
		_log(Poco::Logger::get("")), _type(type), _id(id), _name(name), _duration(duration), _files(files) {

	}

	virtual ~MediaItem(void) {
		for (vector<MediaItemFilePtr>::iterator it = _files.begin(); it != _files.end(); it++) SAFE_DELETE(*it);
		_files.clear();
	}

	const MediaType type() const {
		return _type;
	}

	const bool containsFileType(const MediaType type) {
		for (vector<MediaItemFilePtr>::iterator it = _files.begin(); it != _files.end(); it++) {
			if ((*it)->type() == type) return true;
		}
		return false;
	}

	const string& id() const {
		return _id;
	}

	const string& name() const {
		return _name;
	}

	const int duration() const {
		return _duration;
	}

	const int fileCount() {
		return _files.size();
	}

	const vector<MediaItemFilePtr>& files() {
		return _files;
	}
};


typedef MediaItem* MediaItemPtr;
