#pragma once

namespace ui {

	class MouseListener {
	public:
		MouseListener() {}

		/** ���{�^������ */
		virtual void buttonDownL() {}

		/** ���{�^������ */
		virtual void buttonUpL() {}

		/** �E�{�^������ */
		virtual void buttonDownR() {}

		/** �E�{�^������ */
		virtual void buttonUpR() {}
	};

	typedef MouseListener* MouseListenerPtr;
}
