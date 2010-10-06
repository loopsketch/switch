#pragma once

#include "Transition.h"


/**
 * Slide�g�����W�V����
 */
class SlideTransition: public Transition {
private:
	float _speed;
	int _offsetX;
	int _offsetY;

public:
	SlideTransition(ContentPtr c1, ContentPtr c2, float duration, int offsetX = 0, int offsetY = 32);
	virtual ~SlideTransition();
	virtual void initialize(const DWORD& frame);
	virtual bool process(const DWORD& frame);
};
