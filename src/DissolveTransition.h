#pragma once

#include "Transition.h"


/**
 * Dissolveƒgƒ‰ƒ“ƒWƒVƒ‡ƒ“
 */
class DissolveTransition: public Transition {

public:
	DissolveTransition(ContentPtr c1, ContentPtr c2);
	virtual ~DissolveTransition();
	virtual void initialize(const DWORD& frame);
	virtual bool process(const DWORD& frame);
};
