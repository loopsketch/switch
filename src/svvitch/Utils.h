#pragma once

#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::wstring;
using std::vector;


namespace svvitch {
	/** SJIS>UTF-8に変換 */
	void sjis_utf8(const string& in, string& out);

	/** SJIS>UTF-16に変換 */
	void sjis_utf16(const string& in, wstring& out);

	/** UTF16->SJISに変換 */
	void utf16_sjis(const wstring& wstr, string& out);

	/** UTF-8->SJISに変換 */
	void utf8_sjis(const string& str, string& out);

	/** ファイルのMD5シグネイチャを取得 */
	string md5(const string& file);

	/** 文字列結合 */
	string join(const vector<string>& v, const string& c);

	/** 文字列分割 */
	void split(const string& s, char c, vector<string>& v, int splits = 0);

	/** JSON文字列生成 */
	string formatJSON(const map<string, string>& json);

	/** JSON配列文字列生成 */
	string formatJSONArray(const vector<string>& list);
}
