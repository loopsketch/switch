#pragma once

namespace ui {

	class MouseListener {
	public:
		MouseListener() {}

		/** 左ボタン押下 */
		virtual void buttonDownL() {}

		/** 左ボタン離上 */
		virtual void buttonUpL() {}

		/** 右ボタン押下 */
		virtual void buttonDownR() {}

		/** 右ボタン離上 */
		virtual void buttonUpR() {}
	};

	typedef MouseListener* MouseListenerPtr;
}
