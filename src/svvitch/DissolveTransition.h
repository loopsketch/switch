#pragma once

#include "Transition.h"


/**
 * Dissolveƒgƒ‰ƒ“ƒWƒVƒ‡ƒ“
 */
class DissolveTransition: public Transition {
private:
	float _speed;

public:
	DissolveTransition(ContentPtr c1, ContentPtr c2, float speed);
	virtual ~DissolveTransition();
	virtual void initialize(const DWORD& frame);
	virtual bool process(const DWORD& frame);
};
