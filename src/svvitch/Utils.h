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

/**
 * ���[�e�B���e�B�֐�
 */

namespace svvitch {

	/** �o�[�W���� */
	const string version();

	/**
	 * �t�@�C����Ǎ���Ńo�b�t�@�̃|�C���^��Ԃ��܂��B
	 * �o�b�t�@�͌ďo�����ŉ�����邱�ƁB
	 */
	bool readFile(const string& file, LPVOID* buf);

	/** SJIS>UTF-8�ɕϊ� */
	void sjis_utf8(const string& in, string& out);

	/** SJIS>UTF-16�ɕϊ� */
	void sjis_utf16(const string& in, wstring& out);

	/** UTF16->SJIS�ɕϊ� */
	void utf16_sjis(const wstring& wstr, string& out);

	/** UTF-8->SJIS�ɕϊ� */
	void utf8_sjis(const string& str, string& out);

	/** �T�u�t�H���_���܂ރt�@�C������Ԃ��܂� */
	int fileCount(const Path& path);

	/** �t�@�C����MD5�V�O�l�C�`�����擾 */
	string md5(const Path& path);

	/** �����񌋍� */
	string join(const vector<string>& v, const string& c);

	/** �����񕪊� */
	void split(const string& s, char c, vector<string>& v, int splits = 0);

	/** �������l�L�q������̃p�[�X */
	bool parseMultiNumbers(const string& s, int min, int max, vector<int>& result);

	/** JSON�����񐶐� */
	string formatJSON(const string& s);

	/** JSON�����񐶐� */
	string formatJSON(const map<string, string>& obj);

	/** JSON�z�񕶎��񐶐� */
	string formatJSONArray(const vector<string>& list);

	string trimQuotationMark(const string& s);

	void parseJSON(const string& json, map<string, string>& map);

	void parseJSONArray(const string& json, vector<string>& v);

	/** src�̒��̍Ō��find�ȍ~�̕�������擾���܂� */
	string findLastOfText(const string& src, const string& find);

	vector<int> parseTimes(const string& timeText);

	void rebootWindows(BOOL shutdown = FALSE, BOOL force = FALSE);
}
