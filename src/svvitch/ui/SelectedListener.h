#pragma once

#include <Poco/Logger.h>

namespace ui {

	class SelectedListener {
	protected:
		Poco::Logger& _log;
	public:
		SelectedListener(): _log(Poco::Logger::get("")) {
		}

		/** �I�� */
		virtual void selected() {}
	};

	typedef SelectedListener* SelectedListenerPtr;
}
