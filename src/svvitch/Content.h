#pragma once

#include <string>
#include <vector>
#include <Poco/format.h>
#include <Poco/Logger.h>
#include <Poco/HashMap.h>

#include "Renderer.h"
#include "MediaItem.h"

using std::string;
using std::vector;
using std::wstring;
using Poco::HashMap;


class Content
{
private:
	const string NULL_STRING;

protected:
	Poco::Logger& _log;
	Renderer& _renderer;

//	MediaItemPtr _media;
	string _mediaID;
	bool _playing;

	int _duration;
	int _current;

	int _keycode;
	bool _shift;
	bool _ctrl;

	HashMap<string, string> _properties;
	float _x, _y, _w, _h;

public:
	Content(Renderer& renderer, float x = 0, float y = 0, float w = 0, float h = 0);

	virtual ~Content();

	virtual void initialize();

	virtual bool open(const MediaItemPtr media, const int offset = 0);

	virtual const string opened() const;

	virtual void play();

	virtual void stop();

	virtual const bool playing() const;

	virtual const bool finished();

	/** �t�@�C�����N���[�Y���܂� */
	virtual void close();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	/** 1�t���[����1�x������������� */
	virtual void process(const DWORD& frame);

	/** �`�� */
	virtual void draw(const DWORD& frame);

	/** �v���r���[�`�� */
	virtual void preview(const DWORD& frame);

	/** ���݂̃t���[�� */
	virtual const int current() const;

	/** ����(�t���[����) */
	virtual const int duration() const;

	/** �p�����[�^ */
	virtual void setPosition(float x, float y);

	virtual void getPosition(float& x, float& y);

	virtual void setBounds(float w, float h);

	virtual const bool contains(float x, float y) const;

	void set(const string& key, const string& value);

	void set(const string& key, const float& value);

	const string& get(const string& key) const;

	const float getF(const string& key, const float& defaultValue = 0) const;
};

typedef Content* ContentPtr;
