#include "DissolveTransition.h"


DissolveTransition::DissolveTransition(ContentPtr c1, ContentPtr c2): Transition(c1, c2) {
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
		alpha+=0.05f;
		if (alpha >= 1.0f) {
			alpha = 1.0f;
			finished = true;
		}
		_c2->set("alpha", alpha);
		return finished;
	}
	return false;
}
