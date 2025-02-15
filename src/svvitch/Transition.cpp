#include "Transition.h"

/**
 * Cutトランジション
 */
Transition::Transition(ContentPtr c1, ContentPtr c2): _log(Poco::Logger::get("")), _c1(c1), _c2(c2) {
}

Transition::~Transition() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
}

void Transition::initialize(const DWORD& frame) {
}

bool Transition::process(const DWORD& frame) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	return true;
}

bool Transition::use(ContentPtr c) {
	return _c1 == c || _c2 == c;
}
