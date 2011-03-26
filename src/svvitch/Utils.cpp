#include "Utils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include <Poco/Buffer.h>
#include <Poco/MD5Engine.h>
#include <Poco/DigestStream.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/Logger.h>
#include <Poco/format.h>
#include <Poco/string.h>
#include <Poco/StreamCopier.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/RegularExpression.h>
#include <Poco/NumberParser.h>

using std::copy;
using std::set;
using std::sort;
using std::vector;
using Poco::DigestEngine;
using Poco::File;
using Poco::MD5Engine;
using Poco::DigestOutputStream;
using Poco::StreamCopier;


const string svvitch::version() {
	return "1.01";
}

bool svvitch::readFile(const string& fileName, LPVOID* ref) {
	Poco::Logger& log(Poco::Logger::get(""));
	// dicファイルの読込み
	try {
		File file(fileName);
		if (file.exists()) {
			Poco::FileInputStream is(file.path());
			if (is.good()) {
				LPBYTE buf = new BYTE[(UINT)file.getSize()];
				ZeroMemory(buf, (UINT)file.getSize());
				#define	BUFFER_SIZE	(8192)
				Poco::Buffer<char> buffer(BUFFER_SIZE);
				std::streamsize len = 0;
				is.read(buffer.begin(), BUFFER_SIZE);
				std::streamsize n = is.gcount();
				while (n > 0) {
					CopyMemory(&buf[len], buffer.begin(), n);
					len += n;
					if (is) {
						is.read(buffer.begin(), BUFFER_SIZE);
						n = is.gcount();
					}
					else n = 0;
				}
				is.close();
				log.information(Poco::format("load file size: %d", len));
				*ref = (LPVOID)buf;
				return true;
			} else {
				log.warning(Poco::format("failed not read file: %s", file.path()));
			}
		} else {
			log.warning(Poco::format("file not found: %s", file.path()));
		}
	} catch (Poco::IOException& ex) {
		log.warning(Poco::format("failed read file: %s", ex.displayText()));
	}
	return false;
}

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


int svvitch::fileCount(const Path& path) {
	int count = 0;
	File dir(path);
	if (dir.isDirectory()) {
		vector<File> list;
		dir.list(list);
		for (vector<File>::iterator it = list.begin(); it != list.end(); it++) {
			File f = *it;
			if (f.isDirectory()) {
				count += fileCount(Path(f.path()));
			} else {
				count++;
			}
		}
	} else if (!dir.exists()) {
		count++;
	}
	return count;
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

bool svvitch::parseMultiNumbers(const string& s, int min, int max, vector<int>& result) {
	Poco::Logger& log(Poco::Logger::get(""));
	set<int> num;
	vector<string> datas;
	split(s, ',', datas); // カンマを区切る
	for (vector<string>::const_iterator p = datas.begin(); p != datas.end(); p++) {
		string data = Poco::trim(*p);
		if (!data.empty()) {
			if (data[0] == '-') {
				// 開始省略範囲
				int n;
				if (Poco::NumberParser::tryParse(Poco::trim(data.substr(1)), n)) {
					for (int i = min; i <= n; i++) num.insert(i);
				} else {
					log.warning(Poco::format("parse failed: %s", data));
					return false;
				}

			} else if (data[data.size() - 1] == '-') {
				// 終了省略範囲
				int n;
				if (Poco::NumberParser::tryParse(Poco::trim(data.substr(0, data.size() - 1)), n)) {
					for (int i = n; i <= max; i++) num.insert(i);
				} else {
					log.warning(Poco::format("parse failed: %s", data));
					return false;
				}

			} else if (data.find("-") != string::npos) {
				// 範囲指定
				int i = data.find("-");
				int n1, n2;
				if (!Poco::NumberParser::tryParse(Poco::trim(data.substr(0, i)), n1) || n1 < min) {
					log.warning(Poco::format("parse failed: %s", data));
					return false;
				}
				if (!Poco::NumberParser::tryParse(Poco::trim(data.substr(i + 1)), n2) || n2 > max) {
					log.warning(Poco::format("parse failed: %s", data));
					return false;
				}
				for (int i = n1; i <= n2; i++) num.insert(i);

			} else {
				// 単独数値
				int n = -1;
				if (Poco::NumberParser::tryParse(data, n) && (n >= min && n <= max)) {
					num.insert(n);
				} else {
					log.warning(Poco::format("parse failed: %s", data));
					return false;
				}
			}
		}
	}
	for (set<int>::const_iterator p = num.begin(); p != num.end(); p++) result.push_back(*p);
	sort(result.begin(), result.end());
	return true;
}

string svvitch::formatJSON(const string& s) {
	if (!s.empty()) {
		int i = s.length() - 1;
		if (s.c_str()[0] == '[' && s.c_str()[i] == ']') {
			return s;
		} else if (s.c_str()[0] == '{' && s.c_str()[i] == '}') {
			return s;
		}
		string rep;
		for (i = 0; i < s.length(); ++i) {
			char c = s.at(i);
			if (c == '\"') {
				rep.append("&quot;");
			} else if (c == '\\') {
				rep.append("\\\\");
			} else {
				rep += c;
			}
		}
		return "\"" + rep + "\"";
	}
	return "\"\"";
}

string svvitch::formatJSON(const map<string, string>& obj) {
	vector<string> params;
	for (map<string, string>::const_iterator it = obj.begin(); it != obj.end(); it++) {
		params.push_back(Poco::format("\"%s\":%s", it->first, it->second));
	}
	return Poco::format("{%s}", svvitch::join(params, ","));
}

string svvitch::formatJSONArray(const vector<string>& list) {
	vector<string> params;
	for (vector<string>::const_iterator it = list.begin(); it != list.end(); it++) {
		params.push_back(formatJSON(*it));
	}
	return Poco::format("[%s]", svvitch::join(params, ","));
}

void svvitch::parseJSON(const string& json, map<string, string>& map) {
	string key;
	bool inText = false;
	int start = -1;
	int inArray = 0;
	int inMap = 0;
	string::size_type pos = 0;
	for (; json.length() > pos; pos++) {
		char c = json.at(pos);
		switch (c) {
		case '{':
			if (!inText) {
				// Mapブロック開始
				if (inMap == 1 && start == -1) {
					start = pos;
				}
				inMap++;
			}
			break;
		case '}':
			if (!inText) {
				// Mapブロック終了
				inMap--;
				if (inMap == 0 && start != -1) {
					if (!key.empty()) {
						string value = json.substr(start, pos - start);
						map[trimQuotationMark(key)] = trimQuotationMark(value);
					}
				}
			}
			break;
		case '[':
			if (!inText) {
				// arrayブロック開始
				if (inArray == 0 && start == -1) {
					// Textブロック開始
					start = pos;
				}
				inArray++;
			}
			break;
		case ']':
			if (!inText) {
				// arrayブロック終了
				inArray--;
			}
			break;
		case '\"':
			if (inMap <= 1 && inArray == 0) {
				if (start != -1) {
					// Textブロック終了
					inText = false;
				} else {
					// Textブロック開始
					start = pos;
					inText = true;
				}
			} else {
				// arrayブロック内はスルー
			}
			break;
		case ':':
			if (!inText) {
				// キーブロック終了
				if (inMap <= 1 && inArray == 0) {
					if (start != -1) {
						key = json.substr(start, pos - start);
						start = -1;
					}
				} else {
					// arrayブロック内はスルー
				}
			}
			break;
		case ',':
			if (!inText) {
				// 値ブロック終了
				if (inMap <= 1 && inArray == 0) {
					if (inMap > 0 && !key.empty()) {
						string value = json.substr(start, pos - start);
						map[trimQuotationMark(key)] = trimQuotationMark(value);
						key.clear();
					}
					start = -1;
				} else {
					// arrayブロック内はスルー
				}
			}
			break;
		case ' ':
			// 空白はスルー
			break;
		default:
			if (start == -1) {
				start = pos;
			}
		}
	}
}

void svvitch::parseJSONArray(const string& json, vector<string>& v) {
	string key;
	int start = -1;
	int inArray = 0;
	string::size_type pos = 0;
	for (; json.length() > pos; pos++) {
		char c = json.at(pos);
		switch (c) {
		case '[':
			// arrayブロック開始
			inArray++;
			break;
		case ']':
			// arrayブロック終了
			inArray--;
			if (inArray == 0 && start != -1) {
				string value = json.substr(start, pos - start);
				v.push_back(trimQuotationMark(value));
			}
			break;
		case '\"':
			if (start != -1) {
				// Textブロック終了
			} else {
				// Textブロック開始
				start = pos;
			}
			break;
		case ',':
			// 値ブロック終了
			string value = json.substr(start, pos - start);
			v.push_back(trimQuotationMark(value));
			start = -1;
			break;
		}
	}
}

string svvitch::trimQuotationMark(const string& s) {
	char q = s.at(0);
	switch (q) {
	case '\"':
	//case '[':
	//case '}':
		if (s.at(s.length() - 1) == q) {
			return s.substr(1, s.length() - 2);
		}
	}
	return s;
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

vector<int> svvitch::parseTimes(const string& timeText) {
	Poco::RegularExpression re("[\\s:/]+");
	int pos = 0;
	Poco::RegularExpression::Match match;
	vector<int> time;
	while (re.match(timeText, pos, match) > 0) {
		string s = timeText.substr(pos, match.offset - pos);
		if (s == "*") {
			time.push_back(-1);
		} else {
			time.push_back(Poco::NumberParser::parse(s));
		}
		pos = (match.offset + match.length);
	}
	string s = timeText.substr(pos);
	if (s == "*") {
		time.push_back(-1);
	} else {
		time.push_back(Poco::NumberParser::parse(s));
	}
	return time;
}

void svvitch::rebootWindows(BOOL shutdown, BOOL force) {
	HANDLE hToken;
    LUID Luid;
	HANDLE hProcess = GetCurrentProcess();
	OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &Luid);
	TOKEN_PRIVILEGES tokenNew, tokenPre;
	tokenNew.PrivilegeCount = 1;
	tokenNew.Privileges[0].Luid = Luid;
	tokenNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	DWORD ret;
	AdjustTokenPrivileges(hToken, FALSE, &tokenNew, sizeof(tokenPre), &tokenPre, &ret);

	//case 1:     uFlag = EWX_LOGOFF;     break;
	//case 2:     uFlag = EWX_POWEROFF;   break;
	//case 3:     uFlag = EWX_REBOOT;     break;
	//case 4:     uFlag = EWX_SHUTDOWN;   break;
	UINT flag;
	if (shutdown) {
		// 電源OFF
		flag = EWX_POWEROFF;
	} else {
		// 再起動(default)
		flag = EWX_REBOOT;
	}
	if (force) {
		// プロセス強制終了
        flag |= EWX_FORCE;
    }
	ExitWindowsEx(flag, 0);
}
