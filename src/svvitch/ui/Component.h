#pragma once

#include <string>
#include <vector>
#include <Poco/format.h>
#include <Poco/Logger.h>

#include "UserInterfaceManager.h"

using std::vector;
using std::string;


namespace ui {

	class Component
	{
	private:
		vector<Component*> _child;

	protected:
		Poco::Logger& _log;
		string _name;
		UserInterfaceManagerPtr _uim;

		bool _enabled;
		int _x, _y, _w, _h;
		float _alpha;

		DWORD _color;
		DWORD _background;
		DWORD _border;

	public:
		Component(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha):
			_log(Poco::Logger::get("")), _name(name), _uim(uim), _enabled(true), _x(x), _y(y), _w(w), _h(h), _alpha(alpha),
			_color(0xccffffff), _background(0xcc000000), _border(0xccffffff)
		{
			_uim->addComponent(name, this);
		}

		virtual ~Component(void) {
//			_log.information("delete Component");
			_uim->removeComponent(_name);
		}

		UserInterfaceManagerPtr getUserInterfaceManager() {
			return _uim;
		}

		/** 有効かどうか */
		bool getEnabled() const {
			return _enabled;
		}

		void setEnabled(bool enabled) {
			_enabled = enabled;
		}

		/** カラー設定 */
		void setColor(const DWORD color) {
			_color = color;
		}

		/** 背景色設定 */
		void setBackground(const DWORD color) {
			_background = color;
		}

		/** サイズを設定 */
		void setSize(int w, int h) {
			_w = w;
			_h = h;
		}

		int getY() {
			return _y;
		}

		int getWidth() {
			return _w;
		}

		int getHeight() {
			return _h;
		}

		/** サブコンポーネント追加 */
		void add(Component* ui) {
			_child.push_back(ui);
		}

		/** 指定座標がコンポーネント内かどうか */
		bool contains(int x, int y) {
			return x >= _x && y >= _y && x <= _x + _w && y <= _y + _h;
		}

		/** processの前処理。1フレームに1度だけ処理される */
		virtual void preprocess(const DWORD& frame) {
		}

		/** 1フレームに1度だけ処理される */
		virtual void process(const DWORD& frame) {
		}

		/** processの後処理。1フレームに1度だけ処理される */
		virtual void postprocess(const DWORD& frame) {
		}

		/** 描画 */
		virtual void draw(const DWORD& frame) {
		}

	};

	typedef Component* ComponentPtr;
}
