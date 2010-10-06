#include "DissolveTransition.h"


DissolveTransition::DissolveTransition(ContentPtr c1, ContentPtr c2, float speed): Transition(c1, c2), _speed(speed) {
}

DissolveTransition::~DissolveTransition() {
}

void DissolveTransition::initialize(const DWORD& frame) {
	if (_c1) _c1->set("alpha", "1");
	if (_c2) _c2->set("alpha", "0");
}

bool DissolveTransition::process(const DWORD& frame) {
	if (_c2) {
		bool finished = false;
		float alpha = _c2->getF("alpha");
		alpha += _speed;
		if (alpha >= F(1)) {
			alpha = F(1);
			finished = true;
		}
		_c2->set("alpha", alpha);
		return finished;
	}
	return false;
}
