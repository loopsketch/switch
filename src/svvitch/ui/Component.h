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
		Component(string name, UserInterfaceManagerPtr uim, int x, int y, int w, int h, float alpha);

		virtual ~Component(void);

		UserInterfaceManagerPtr getUserInterfaceManager();

		/** 有効かどうか */
		bool getEnabled() const;

		void setEnabled(bool enabled);

		/** カラー設定 */
		void setColor(const DWORD color);

		/** 背景色設定 */
		void setBackground(const DWORD color);

		/** 位置を設定 */
		void setPosition(int x, int y);

		/** サイズを設定 */
		void setSize(int w, int h);

		/** 位置を取得 */
		virtual int getX();
		virtual int getY();

		/** サイズを取得 */
		virtual int getWidth();
		virtual int getHeight();

		/** サブコンポーネント追加 */
		void add(Component* ui);

		/** 指定座標がコンポーネント内かどうか */
		bool contains(int x, int y);

		/** processの前処理。1フレームに1度だけ処理される */
		virtual void preprocess(const DWORD& frame);

		/** 1フレームに1度だけ処理される */
		virtual void process(const DWORD& frame);

		/** processの後処理。1フレームに1度だけ処理される */
		virtual void postprocess(const DWORD& frame);

		/** 描画 */
		virtual void draw(const DWORD& frame);
	};

	typedef Component* ComponentPtr;
}
