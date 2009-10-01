#pragma once

#include "Content.h"
#include <vector>

using std::vector;


class UI: public Content
{
private:
	vector<UI*> _child;

protected:
	bool _mouseOver;

public:
	UI(Renderer& renderer, int x, int y, int w, int h): Content(renderer, x, y, w, h) {
	}

	virtual ~UI(void) {
	}

	void add(UI* ui) {
		_child.push_back(ui);
	}

	virtual void mouseOver() {
		// UI�Ƀ}�E�X���d�Ȃ��Ă���ꍇ
		_mouseOver = true;
	}

	virtual void mouseOut() {
		// UI�Ƀ}�E�X���d�Ȃ��Ă���ꍇ
		_mouseOver = false;
	}
};
