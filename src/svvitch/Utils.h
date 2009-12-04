#pragma once

#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::wstring;
using std::vector;


namespace svvitch {
	/** SJIS>UTF-8�ɕϊ� */
	void sjis_utf8(const string& in, string& out);

	/** SJIS>UTF-16�ɕϊ� */
	void sjis_utf16(const string& in, wstring& out);

	/** UTF16->SJIS�ɕϊ� */
	void utf16_sjis(const wstring& wstr, string& out);

	/** UTF-8->SJIS�ɕϊ� */
	void utf8_sjis(const string& str, string& out);

	/** �t�@�C����MD5�V�O�l�C�`�����擾 */
	string md5(const string& file);

	/** �����񌋍� */
	void join(const vector<string>& v, char c, string& s);

	/** �����񕪊� */
	void split(const string& s, char c, vector<string>& v, int splits = 0);

	/** JSON�����񐶐� */
	void formatJSON(const map<string, string>& json, string& s);

	/** JSON�z�񕶎��񐶐� */
	void formatJSONArray(const vector<string>& list, string& s);
}
