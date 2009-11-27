#include "Utils.h"

#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <Poco/MD5Engine.h>
#include <Poco/DigestStream.h>
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
	// UTF-8‚ðUTF-16‚É•ÏŠ·
	Poco::UnicodeConverter::toUTF16(str, wstr);
	utf16_sjis(wstr, out); // UTF-16‚ðƒVƒtƒgJIS‚É•ÏŠ·
}


string svvitch::md5(const string& file) {
	std::ifstream istr(file.c_str(), std::ios::binary);
	if (istr) {
		MD5Engine md5;
		DigestOutputStream dos(md5);
		StreamCopier::copyStream(istr, dos);
		dos.close();
		return DigestEngine::digestToHex(md5.digest());
	}
	return string("");
}


void svvitch::split(const string& s, char c, vector<string>& v) {
	string::size_type i = 0;
	string::size_type j = s.find(c);

	while (j != string::npos) {
		v.push_back(s.substr(i, j - i));
		i = ++j;
		j = s.find(c, j);

		if (j == string::npos) v.push_back(s.substr(i, s.length()));
	}
}
