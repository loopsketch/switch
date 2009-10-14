#pragma once

#include "Transition.h"


/**
 * Dissolve�g�����W�V����
 */
class DissolveTransition: public Transition {

public:
	DissolveTransition(ContentPtr c1, ContentPtr c2);
	virtual ~DissolveTransition();
	virtual void initialize(const DWORD& frame);
	virtual bool process(const DWORD& frame);
};
