#pragma once

#include "Content.h"
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <vector>

using std::vector;


/**
 * コンテナクラス.
 * 複数のContentを含むContentです
 */
class Container: public Content
{
private:
	Poco::FastMutex _lock;
	vector<ContentPtr> _list;

	bool _initialized;

public:
	Container(Renderer& renderer);
	virtual ~Container();

	void initialize();

	void add(ContentPtr c);

	ContentPtr operator[](int i);

	ContentPtr get(int i);

	int size();

	const string opened();

	void play();

	void pause();

	void stop();

	bool useFastStop();

	void rewind();

	const bool finished();

	void notifyKey(const int keycode, const bool shift, const bool ctrl);

	void process(const DWORD& frame);

	void draw(const DWORD& frame);

	void preview(const DWORD& frame);

	const int current();

	const int duration();

	void setProperty(const string& key, const string& value);
};

typedef Container* ContainerPtr;