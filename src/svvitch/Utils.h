#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include <Poco/Path.h>

using std::map;
using std::string;
using std::wstring;
using std::vector;
using Poco::Path;


namespace svvitch {

	/** バージョン */
	const string version();

	/**
	 * ファイルを読込んでバッファのポインタを返します。
	 * バッファは呼出し側で解放すること。
	 */
	bool readFile(const string& file, LPVOID* buf);

	/** SJIS>UTF-8に変換 */
	void sjis_utf8(const string& in, string& out);

	/** SJIS>UTF-16に変換 */
	void sjis_utf16(const string& in, wstring& out);

	/** UTF16->SJISに変換 */
	void utf16_sjis(const wstring& wstr, string& out);

	/** UTF-8->SJISに変換 */
	void utf8_sjis(const string& str, string& out);

	/** サブフォルダを含むファイル数を返します */
	int fileCount(const Path& path);

	/** ファイルのMD5シグネイチャを取得 */
	string md5(const Path& path);

	/** 文字列結合 */
	string join(const vector<string>& v, const string& c);

	/** 文字列分割 */
	void split(const string& s, char c, vector<string>& v, int splits = 0);

	/** JSON文字列生成 */
	string formatJSON(const string& s);

	/** JSON文字列生成 */
	string formatJSON(const map<string, string>& obj);

	/** JSON配列文字列生成 */
	string formatJSONArray(const vector<string>& list);

	string trimQuotationMark(const string& s);

	void parseJSON(const string& json, map<string, string>& map);

	void parseJSONArray(const string& json, vector<string>& v);

	/** srcの中の最後のfind以降の文字列を取得します */
	string findLastOfText(const string& src, const string& find);

	vector<int> parseTimes(const string& timeText);
}
