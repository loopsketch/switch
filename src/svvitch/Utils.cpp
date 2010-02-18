#include "Utils.h"

#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <Poco/MD5Engine.h>
#include <Poco/DigestStream.h>
#include <Poco/format.h>
#include <Poco/StreamCopier.h>
#include <Poco/UnicodeConverter.h>

using std::vector;
using Poco::DigestEngine;
using Poco::MD5Engine;
using Poco::DigestOutputStream;
using Poco::StreamCopier;


void svvitch::sjis_utf8(const string& in, string& out) {
	wstring wstring;
	sjis_utf16(in, wstring);
	Poco::UnicodeConverter::toUTF8(wstring, out);
}

void svvitch::sjis_utf16(const string& in, wstring& out) {
	int len = ::MultiByteToWideChar(CP_ACP, 0, in.c_str(), -1, NULL, 0);
	if (len > 0) { 
		vector<wchar_t> utf16(len);
		if (::MultiByteToWideChar(CP_ACP, 0, in.c_str(), -1, &utf16[0], len)) {
			out = &utf16[0];
		}
		utf16.clear();
	} else {
		out = L"";
	}
}


void svvitch::utf16_sjis(const wstring& wstr, string& out) {
	int len = ::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	if (len > 0) {
		vector<char> sjis(len);
		if (::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &sjis[0], len, NULL, NULL)) {
			out = &sjis[0];
		}
		sjis.clear();
	} else {
		out.clear();
	}
}


void svvitch::utf8_sjis(const string& str, string& out) {
	std::wstring wstr;
	// UTF-8をUTF-16に変換
	Poco::UnicodeConverter::toUTF16(str, wstr);
	utf16_sjis(wstr, out); // UTF-16をシフトJISに変換
}


string svvitch::md5(const Path& path) {
	std::wstring wfile;
	Poco::UnicodeConverter::toUTF16(path.toString(), wfile);
	std::ifstream is(wfile.c_str(), std::ios::binary);
	if (is.good()) {
		MD5Engine md5;
		DigestOutputStream dos(md5);
		StreamCopier::copyStream(is, dos);
		dos.close();
		return DigestEngine::digestToHex(md5.digest());
	}
	return string("");
}

string svvitch::join(const vector<string>& v, const string& c) {
	string s;
	for (vector<string>::const_iterator p = v.begin(); p != v.end(); p++) {
		s += *p;
		if (p != v.end() -1) s += c;
	}
	return s;
}

void svvitch::split(const string& s, char c, vector<string>& v, int splits) {
	string::size_type pos = 0;
	string::size_type j = s.find(c);

	int count = 0;
	while (j != string::npos) {
		if (splits > 0 && (splits - 1) == count) {
			v.push_back(s.substr(pos));
			break;
		}
		v.push_back(s.substr(pos, j - pos));
		pos = ++j;
		j = s.find(c, j);

		if (j == string::npos) v.push_back(s.substr(pos, s.length()));
		count++;
	}
	if (v.empty()) v.push_back(s);
}

string svvitch::formatJSON(const map<string, string>& json) {
	vector<string> params;
	for (map<string, string>::const_iterator it = json.begin(); it != json.end(); it++) {
		params.push_back(Poco::format("\"%s\":%s", it->first, it->second));
	}
	return Poco::format("{%s}", svvitch::join(params, ","));
}

string svvitch::formatJSONArray(const vector<string>& list) {
	vector<string> params;
	for (vector<string>::const_iterator it = list.begin(); it != list.end(); it++) {
		params.push_back(*it);
	}
	return Poco::format("[%s]", svvitch::join(params, ","));
}

void svvitch::parseJSON(const string& json, map<string, string>& map) {
	string key;
	int inText = -1;
	int inArray = 0;
	int inMap = 0;
	string::size_type pos = 0;
	for (; json.length() > pos; pos++) {
		char c = json.at(pos);
		switch (c) {
		case '{':
			// Mapブロック開始
			inMap++;
			break;
		case '}':
			// Mapブロック終了
			inMap--;
			break;
		case '[':
			// arrayブロック開始
			if (inArray == 0) {
				// Textブロック開始
				inText = pos;
			}
			inArray++;
			break;
		case ']':
			// arrayブロック終了
			inArray--;
			break;
		case '\"':
			if (inMap <= 1 && inArray == 0) {
				if (inText != -1) {
					// Textブロック終了
				} else {
					// Textブロック開始
					inText = pos;
				}
			} else {
				// arrayブロック内はスルー
			}
			break;
		case ':':
			// キーブロック終了
			if (inMap <= 1 && inArray == 0) {
				if (inText != -1) {
					key = json.substr(inText, pos - inText);
					inText = -1;
				}
			} else {
				// arrayブロック内はスルー
			}
			break;
		case ',':
			// 値ブロック終了
			if (inMap <= 1 && inArray == 0) {
				if (inMap > 0 && !key.empty()) {
					string value = json.substr(inText, pos - inText);
					map[key] = value;
					key.clear();
				}
				inText = -1;
			} else {
				// arrayブロック内はスルー
			}
			break;
		}
	}
	// 最後の要素終了
	if (!key.empty()) {
		string value = json.substr(inText, pos - inText);
		map[key] = value;
	}
}

void svvitch::parseJSONArray(const string& json, vector<string>& v) {
}

string svvitch::findLastOfText(const string& src, const string& find) {
	string s;
	int i = src.find_last_of(find);
	if (i == string::npos) {
		s = src;
	} else {
		s = src.substr(i + 1);
	}
	return s;
}

