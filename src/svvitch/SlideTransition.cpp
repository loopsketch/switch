#include "SlideTransition.h"


SlideTransition::SlideTransition(ContentPtr c1, ContentPtr c2, float speed, int offsetX, int offsetY):
	Transition(c1, c2), _speed(speed), _offsetX(offsetX), _offsetY(offsetY)
{
}

SlideTransition::~SlideTransition() {
}

void SlideTransition::initialize(const DWORD& frame) {
	if (_c1) {
		float x, y;
		_c1->getPosition(x, y);
		_c1->setPosition(0, 0);
	}
	if (_c2) {
		_c2->setPosition(_offsetX, _offsetY);
	}
}

bool SlideTransition::process(const DWORD& frame) {
	if (_c1) {
		float x1, y1;
		_c1->getPosition(x1, y1);
		if (y1 <= -_offsetY) return true;
		y1 -= _speed;
		_c1->setPosition(x1, y1);
	}
	if (_c2) {
		float x2, y2;
		_c2->getPosition(x2, y2);
		y2 -= _speed;
		_c2->setPosition(x2, y2);
	}
	return false;
}
