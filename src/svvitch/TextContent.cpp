#include "TextContent.h"

#include <Poco/FileStream.h>
#include <Poco/LineEndingConverter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>

#include <gdiplus.h>

#include "Utils.h"

using namespace Gdiplus;


TextContent::TextContent(Renderer& renderer, int splitType, float x, float y, float w, float h): Content(renderer, splitType, x, y, w, h),
	_texture(NULL), _referencedText(NULL), _async(false)
{
	initialize();
}

TextContent::~TextContent() {
	initialize();
}

void TextContent::initialize() {
	Poco::ScopedLock<Poco::FastMutex> lock(_initializeLock);
	close();
	_dx = 0;
	_dy = 0;
}

bool TextContent::open(const MediaItemPtr media, const int offset) {
	initialize();
	Poco::RegularExpression re("^(sjis|shift_jis|shiftjis|ms932)$", Poco::RegularExpression::RE_CASELESS + Poco::RegularExpression::RE_EXTENDED);

	bool valid = false;
	if (offset != -1 && offset < media->fileCount()) {
		MediaItemFile mif =  media->files().at(offset);
		if (mif.type() == MediaTypeText) {
			_textFont = mif.getProperty("font");
			if (_textFont.empty()) _textFont = config().textFont;
			_textHeight = mif.getNumProperty("fh", config().textHeight);
			_desent = 0;
			_c1 = mif.getHexProperty("c1", 0xffffffff);
			_c2 = mif.getHexProperty("c2", 0xffcccccc);
			_b1 = mif.getHexProperty("b1", 0xff000000);
			_b2 = mif.getHexProperty("b2", 0x00cccccc);
			_borderSize1 = mif.getFloatProperty("bs1", 4);
			_borderSize2 = mif.getFloatProperty("bs2", 0);
			_textStyle = mif.getProperty("style", config().textStyle);

			_x = mif.getNumProperty("x", 0);
			_y = mif.getNumProperty("y", 0);
			_w = mif.getNumProperty("w", config().stageRect.right);
			_h = mif.getNumProperty("h", config().stageRect.bottom);
			_cx = mif.getNumProperty("cx", _x);
			_cy = mif.getNumProperty("cy", _y);
			_cw = mif.getNumProperty("cw", _w);
			_ch = mif.getNumProperty("ch", _h);
			_move = mif.getProperty("move");
			_frameWait = mif.getNumProperty("fw", 0);
			_async = mif.getProperty("async") == "true";
			//_dx = mif.getFloatProperty("dx", F(0));
			//_dy = mif.getFloatProperty("dy", F(0));
			_align = mif.getProperty("align");
			_fitBounds = mif.getProperty("fit") == "true";
			string pos = Poco::format("(%0.1hf,%0.1hf) %0.1hfx%0.1hf dx:%0.1hf dy:%0.1hf", _x, _y, _w, _h, _dx, _dy);
			pos += Poco::format(" fw:%d", _frameWait);
			_log.information(Poco::format("text: [%s] %s", _textFont, pos));

			if (!mif.file().empty() && mif.file().find("$") == string::npos) {
				string text;
				try {
					Poco::RegularExpression::Match m;
					bool isSJIS = re.match(mif.getProperty("encoding"), m);
					Poco::FileInputStream fis(Path(mif.file()).absolute(config().dataRoot).toString(), std::ios::in);
					try {
						Poco::InputLineEndingConverter ilec(fis);
						char line[1024];
						int linenum = 1;
						while (!ilec.eof()) {
							line[0] = '\0';
							ilec.getline(line, sizeof(line));
							if (!text.empty()) text += "          ";
							if (isSJIS) {
								// sjis
								wstring wline;
								svvitch::sjis_utf16(string(line), wline);
								string utf8;
								Poco::UnicodeConverter::toUTF8(wline, utf8);
								text = Poco::cat(text, utf8);
							} else {
								// UTF-8
								text = Poco::cat(text, string(line));
							}
							linenum++;
						}
						_log.information(Poco::format("lines: %d %s", linenum, text));

						drawTexture(text);
						valid = true;
					} catch (Poco::Exception& ex) {
						_log.warning(Poco::format("I/O error: %s", ex.displayText()));
					}
					fis.close();
				} catch (Poco::PathNotFoundException& ex) {
					_log.warning(Poco::format("file not found: %s", mif.file()));
				} catch (Poco::Exception& ex) {
					_log.warning(Poco::format("failed read text: %s", ex.displayText()));
				}
			} else {
				// ファイル指定無し
				_log.information("text template mode");
				valid = true;
			}
		} else {
			_log.warning("failed type error");
		}
	}

	if (valid) {
		_mediaID = media->id();
		set("alpha", 1.0f);
		return true;
	}
	return false;
}

void TextContent::setReference(TextContent* text) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_referencedText = text;
	_log.information("set reference text content");
}

void TextContent::play() {
	if (_move.find("roll-left-") == 0) {
		_x = _cx + _cw;
		_sx = _x;
		//_y = 0;
		int dx = 0;
		Poco::NumberParser::tryParse(_move.substr(10), dx);
		_dx = -dx;
		_log.debug(Poco::format("move: scroll-left: %0.1hf", _dx));
	} else if (_move.find("roll-up-") == 0) {
		_y = _cy + _ch;
		_sy = _y;
		int dy = 0;
		Poco::NumberParser::tryParse(_move.substr(8), dy);
		_dy = -dy;
		_log.debug(Poco::format("move: scroll-up: %0.1hf", _dy));
	}
	_playing = true;
}

void TextContent::stop() {
	_playing = false;
}

const bool TextContent::finished() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_move.empty() || _async) {
		return true;
	}
	return !_playing;
}

/** ファイルをクローズします */
void TextContent::close() {
	_mediaID.clear();
	LPDIRECT3DTEXTURE9 old = NULL;
	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_referencedText) {
			_referencedText = NULL;
		} else {
			old = _texture;
		}
		_texture = NULL;
	}
	SAFE_RELEASE(old);
	_iw = 0;
	_ih = 0;
	_tw = 0;
	_th = 0;
}

/** 1フレームに1度だけ処理される */
void TextContent::process(const DWORD& frame) {
	if (_align == "center") {
		_ax = (_w - _iw) / 2;
	} else if (_align == "right") {
		_ax = _w - _iw;
	} else {
		_ax = 0;
	}

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		if (_referencedText) {
			_texture = _referencedText->_texture;
			_ax = _referencedText->_ax;
			_iw = _referencedText->_iw;
			_ih = _referencedText->_ih;
			_tw = _referencedText->_tw;
			_th = _referencedText->_th;
		}
	}
	if (!_mediaID.empty() && _texture && _playing) {
		if (!_move.empty()) {
			if (_frameWait <= 0 || (frame % _frameWait) == 0) {
				_x += _dx;
				_y += _dy;
			}
			if (!_async) {
				if (_move.find("roll-left-") == 0) {
					if (_dx != 0) {
						_duration = abs((_sx + _iw) / _dx);
						int current = abs((_sx - _x) / _dx);
						int dt = abs((_x - (_cx - _iw)) / _dx);
						const int fps = 60;
						unsigned long cu = current / fps;
						unsigned long re = dt / fps;
						string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, (current % fps) / 2);
						string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, (dt % fps) / 2);
						set("time", Poco::format("%s %s", t1, t2));
						set("time_current", t1);
						set("time_remain", t2);
					}
					if (_x < (_cx - _iw)) {
						// _log.information(Poco::format("text move finished: %hf %d %d", _x, _cx, _iw));
						_dx = 0;
						_move.clear();
						//_playing = false;
					}
				} else if (_move.find("roll-up-") == 0) {
					if (_dy != 0) {
						_duration = abs((_sy + _ih) / _dy);
						int current = abs((_sy - _y) / _dy);
						int dt = abs((_y - (_cy - _ih)) / _dy);
						const int fps = 60;
						unsigned long cu = current / fps;
						unsigned long re = dt / fps;
						string t1 = Poco::format("%02lu:%02lu:%02lu.%02d", cu / 3600, cu / 60, cu % 60, (current % fps) / 2);
						string t2 = Poco::format("%02lu:%02lu:%02lu.%02d", re / 3600, re / 60, re % 60, (dt % fps) / 2);
						set("time", Poco::format("%s %s", t1, t2));
						set("time_current", t1);
						set("time_remain", t2);
					}
					if (_y < (_cy - _ih)) {
						_dy = 0;
						_move.clear();
					}
				} else {
					_duration = 0;
				}
			} else {
				if (_x < (_cx - _iw)) _x = _cx + _cw;
				_duration = 0;
			}
		}
		//_x+=_dx;
		//_y+=_dy;
		_current++;
		if (_duration < _current) _current = _duration;
	}
}

/** 描画 */
void TextContent::draw(const DWORD& frame) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_mediaID.empty() && _texture && _playing) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		float alpha = getF("alpha", 1.0f);
		DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
		int cw = config().splitSize.cx;
		int ch = config().splitSize.cy;

		switch (_splitType) {
		case 1:
			{
				device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
				RECT scissorRect;
				device->GetScissorRect(&scissorRect);
				float x = _x + _ax;
				float y = _y;
				int ix = 0, sx = 0, sy = 0, dx = (int)x / (cw * config().splitCycles) * cw, dxx = (int)fmod(x, cw), dy = ch * ((int)x / cw) % (config().splitCycles * ch);
				int cww = 0;
				int chh = (ch > _ih)?_ih:ch;
				int clipX = _cx;
				while (dx < config().mainRect.right) {
					RECT rect = {dx, dy, dx + cw, dy + chh};
					int cx = dx * config().splitCycles + dy / ch * cw; // cx=実際の映像の横位置
					if (cx > config().stageRect.right) break;
					int cxx = _cx - cx;
					if (cxx > cw) {
						SetRect(&rect, 0, 0, 0, 0);
					} else if (cxx >= 0) {
						SetRect(&rect, dx + cxx, rect.top, rect.right, rect.bottom);
					}
					cxx = _cx + _cw - cx;
					if (cxx < 0) {
						SetRect(&rect, 0, 0, 0, 0);
					} else if (cxx <= cw) {
						SetRect(&rect, rect.left, rect.top, dx + cxx, rect.bottom);
					}
					device->SetScissorRect(&rect);
//					if ((sx + cw - dxx) >= _tw) {
					if ((sx + cw - dxx) >= _tw) {
						// ソースの横がはみ出る()
						cww = _tw - sx;
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _texture, 0, col, col, col, col);
						// _renderer.drawFontTextureText(dx + dxx, y + dy, 10, 10, 0xffff0000, Poco::format("%d,%d %d %d %d %d", sx, sy, cw, dxx, _tw, cww));
						sx = 0;
						sy += _ih;
						if (sy < _th) {
							// srcを折り返してdstの残りに描画
							if (_th - sy < ch) chh = _th - sy;
							_renderer.drawTexture(dx + dxx + cww, y + dy, cw - cww, chh, sx, sy, cw - cww, chh, _texture, 0, col, col, col, col);
							// _renderer.drawFontTextureText(dx + dxx + cww, y + dy, 12, 12, 0xff00ff00, Poco::format("t%d,%d", sx, sy));
							sx += cw - cww;
							ix += cw;
							dxx = cww + cw - cww;
//							if (ix >= _iw) _log.information(Poco::format("image check1: %d,%d %d", dx, dy, dxx));
						} else {
							// dstの途中でsrcが全て終了
							dxx = _iw - ix;
							ix = _iw;
//							if (ix >= _iw) _log.information(Poco::format("image check2: %d,%d %d", dx, dy, dxx));
						}
					} else {

						if (_iw - ix < (cw - dxx)) {
							cww = _iw - ix;
						} else {
							cww = cw - dxx;
						}
						if (sx + cww >= _tw) {
							cww = _tw - sx;
						}
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _texture, 0, col, col, col, col);
						// _renderer.drawFontTextureText(dx + dxx, y + dy, 12, 12, 0xffffcc00, Poco::format("%d,%d", sx, sy));
						sx += cww;
						ix += cww;
//						dxx = cww;
					}
					if (ix >= _iw) {
						break;
//						dxx = cw - cww;
//						sx = 0;
//						ix = 0;
					}
					dxx = 0;
					dy += ch;
					if (dy >= config().splitCycles * ch) {
						dx += cw;
						dy = 0;
					}
				}
				device->SetScissorRect(&scissorRect);
			}
			break;

		case 2:
			{
				device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
				RECT scissorRect;
				device->GetScissorRect(&scissorRect);
				int cww = cw;
				int chh = ch;
				int sw = config().stageRect.right / cw;
				int sh = config().stageRect.bottom / ch;
				for (int sy = 0; sy < sh; sy++) {
					int ox = (sy % 2) * cw * 8 + (L(_y / ch) % 2) * cw * 8 + config().stageRect.left;
					int oy = (sy / 2) * ch * 4 + (L(_y / ch) / 2) * ch * 4 + config().stageRect.top;
					for (int sx = 0; sx < sw; sx++) {
//						if (_x >= sx * cw + cw || _x + _tw < sx * cw) continue;
						int dx = (sx / 4) * cw;
						int dy = ch * 3 - (sx % 4) * ch;
						RECT rect = {ox + dx, oy + dy, ox + dx + cw, oy + dy + ch};
						device->SetScissorRect(&rect);
						int tx = sx * cw -_x;
						int ty = -_dy; //sy * ch -_y;
						int tcw = cww;
						int tch = chh;
						if (_tw - tx < cww) tcw = _tw - tx;
//						if (_th - ty < chh) tch = _th - ty;
						if (tcw > 0 && tch > 0) _renderer.drawTexture(ox + dx, oy + dy, tcw, tch, tx, ty, tcw, tch, _texture, 0, col, col, col, col);
					}
				}
				device->SetScissorRect(&scissorRect);
//				_renderer.drawFontTextureText(0, conf->subRect.bottom - 40, 12, 16, 0xffcccccc, Poco::format("text: %d,%d", tx, ty));
			}
			break;
		default:
			{
				float alpha = getF("alpha");
				DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
				if (_fitBounds) {
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
					_renderer.drawTexture(_x, _y, _w, _ih, _texture, 0, col, col, col, col);
					device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
					device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				} else {
					if (_ih != _th) {
						const int c = _th / _ih;
						for (int i = 0; i < c; i++) {
							_renderer.drawTexture(_x + _ax + i * _tw, _y, _tw, _ih, 0, i * _ih, _tw, _ih, _texture, 0, col, col, col, col);
						}
					} else {
						_renderer.drawTexture((int)_x + _ax, (int)_y, _texture, 0, col, col, col, col);
					}
				}
			}
		}
	} else {
		if (get("prepare") == "true") {
			int sy = getF("itemNo") * 20;
			DWORD col = 0xccffffff;
			_renderer.drawTexture(700, 600 + sy, 324, 20, _texture, 0, col, col, col, col);
		}
	}
}

int TextContent::getTextWidth() {
	return _iw;
}

int TextContent::getTextHeight() {
	return _ih;
}

void TextContent::setFitBounds(bool fit) {
	_fitBounds = fit;
}

void TextContent::setColor(DWORD c1, DWORD c2) {
	_c1 = c1;
	_c2 = c2;
}

void TextContent::setBorder1(int size, DWORD col) {
	_borderSize1 = size;
	_b1 = col;
}

void TextContent::setBorder2(int size, DWORD col) {
	_borderSize2 = size;
	_b2 = col;
}

void TextContent::setFont(string font) {
	_textFont = font;
}

void TextContent::setFontHeight(int height, int desent) {
	_textHeight = height;
	_desent = desent;
}

void TextContent::setTextStyle(string style) {
	_textStyle = style;
}

void TextContent::setAlign(string align) {
	_align = align;

	if (_align == "center") {
		_ax = (_w - _iw) / 2;
	} else if (_align == "right") {
		_ax = _w - _iw;
	} else {
		_ax = 0;
	}
	_log.information(Poco::format("align change: %dx%d %dx%d align: %d", _iw, _ih, _tw, _th, _ax));
}

void TextContent::drawTexture(string text) {
	_text = text;
	Rect rect(0, 0, 0, 0);
	{
		Bitmap bitmap(1, 1, PixelFormat32bppARGB);
		drawText(text, bitmap, rect);
	}
	{
		int x = rect.X;
		int y = rect.Y;
		int w = rect.Width;
		int h = rect.Height;
		rect.Width = w - x;		// rectの領域をx/y=0で作り直す
		rect.Height = h - y;	// ただしx/yはクリアせずそのまま引き渡すことで、biasとして使用する
		_log.debug(Poco::format("bitmap: %d,%d %dx%d", x, y, w, h));
//		int sh = config().stageRect.bottom;
		LPDIRECT3DTEXTURE9 texture = _renderer.createTexture(w, h, D3DFMT_A8R8G8B8);
		int tw = 0;
		int th = 0;
		int iw = 0;
		int ih = 0;
		if (texture) {
			D3DSURFACE_DESC desc;
			HRESULT hr = texture->GetLevelDesc(0, &desc);
			tw = desc.Width;
			th = desc.Height;
			_log.information(Poco::format("text texture: %dx%d", tw, th));
//			th = sh;
		} else {
			// 指定サイズのテクスチャが作れない
			tw = config().imageSplitWidth;
			th = h;
		}
		if (w > tw || !texture) {
			// テクスチャの幅の方が小さい場合、テクスチャを折返しで作る
			SAFE_RELEASE(texture);
			Bitmap bitmap(w, h, PixelFormat32bppARGB);
			drawText(text, bitmap, rect);
			texture = _renderer.createTexture(tw, th * ((w + tw - 1) / tw), D3DFMT_A8R8G8B8);
			if (!texture) return;
			D3DSURFACE_DESC desc;
			HRESULT hr = texture->GetLevelDesc(0, &desc);
			_log.information(Poco::format("text texture(with turns %dx%d): %ux%u", tw, th, desc.Width, desc.Height));
			_renderer.colorFill(texture, 0x00000000);
			D3DLOCKED_RECT lockRect;
			hr = texture->LockRect(0, &lockRect, NULL, 0);
			if (SUCCEEDED(hr)) {
				Bitmap dst(desc.Width, desc.Height, lockRect.Pitch, PixelFormat32bppARGB, (BYTE*)lockRect.pBits);
				Graphics g(&dst);
				int y = 0;
				for (int x = 0; x < w; x+=tw) {
					Rect rect(0, y, tw, th);
					g.SetClip(rect);
					g.DrawImage(&bitmap, -x, y);
					y += th;
				}
				g.Flush();
				hr = texture->UnlockRect(0);
//				_w = 1024;
				iw = w;
				ih = h;
				tw = desc.Width;
				th = desc.Height;
			}

		} else {
			// 折返し無し
			_renderer.colorFill(texture, 0x00000000);
			D3DLOCKED_RECT lockRect;
			HRESULT hr = texture->LockRect(0, &lockRect, NULL, 0);
			if (SUCCEEDED(hr)) {
				Bitmap bitmap(tw, th, lockRect.Pitch, PixelFormat32bppARGB, (BYTE*)lockRect.pBits);
				drawText(text, bitmap, rect);
				hr = texture->UnlockRect(0);
				_log.information(Poco::format("draw text texture: %dx%d", tw, th));
//				_w = (float)rect.Width;
				iw = tw;
				ih = th;
			}
		}
		LPDIRECT3DTEXTURE9 old = NULL;
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_texture) old = _texture;
			_texture = texture;
			// D3DXSaveTextureToFile(L"test_text.png", D3DXIFF_PNG, _texture, NULL);
			_iw = iw;
			_ih = ih;
			_tw = tw;
			_th = th;
		}
		SAFE_RELEASE(old);
	}

	if (_align == "center") {
		_ax = (_w - _iw) / 2;
	} else if (_align == "right") {
		_ax = _w - _iw;
	} else {
		_ax = 0;
	}
	_log.information(Poco::format("texture updated: %dx%d %dx%d align: %d", _iw, _ih, _tw, _th, _ax));
}

void TextContent::drawText(string text, Bitmap& bitmap, Rect& rect) {
	int x = 0;
	int y = 0;
	int h = rect.GetBottom() - rect.GetTop();
	if (rect.GetRight() - rect.GetLeft() != 0 || rect.GetBottom() - rect.GetTop() != 0) {
		x = -rect.X;
		y = -rect.Y;
	}

	Graphics g(&bitmap);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintAntiAlias);

	Gdiplus::FontFamily* ff = NULL;
	_renderer.getPrivateFontFamily(_textFont, &ff);
	if (!ff) {
		// アプリのフォントが利用できない場合システムから取得してみる
		std::wstring wfontFamily;
		Poco::UnicodeConverter::toUTF16(_textFont, wfontFamily);
		Font f(wfontFamily.c_str(), _textHeight);
		Gdiplus::FontFamily fontFamily;
		f.GetFamily(&fontFamily);
		ff = fontFamily.Clone();
		WCHAR wname[64] = L"";
		ff->GetFamilyName(wname);
		string name;
		Poco::UnicodeConverter::toUTF8(wname, name);
		_log.information(Poco::format("installed font: %s", name));
	}
	Gdiplus::FontStyle style;
	if (_textStyle == "bold") {
		style = Gdiplus::FontStyleBold;
	} else if (_textStyle == "italic") {
		style = Gdiplus::FontStyleItalic;
	} else if (_textStyle == "bolditalic") {
		style = Gdiplus::FontStyleBoldItalic;
	} else {
		style = Gdiplus::FontStyleRegular;
	}

	Poco::RegularExpression re1("\\r|\\n");
	re1.subst(text, "          ", Poco::RegularExpression::RE_GLOBAL);
	std::wstring wtext;
	Poco::UnicodeConverter::toUTF16(text, wtext);
	size_t len = wcslen(wtext.c_str());
	int bh = _borderSize1 + _borderSize2;
	GraphicsPath path;
	path.AddString(wtext.c_str(), len, ff, style, _textHeight, Point(x, y + bh), StringFormat::GenericDefault());
	if (_borderSize2 > F(0)) {
		SolidBrush borderBrush2(_b2);
		Pen pen2(&borderBrush2, bh);
		pen2.SetLineJoin(LineJoinRound);
		g.DrawPath(&pen2, &path);
	}
	SolidBrush borderBrush1(_b1);
	Pen pen1(&borderBrush1, _borderSize1);
	if (_borderSize1 > F(0)) {
		pen1.SetLineJoin(LineJoinRound);
		g.DrawPath(&pen1, &path);
	}
	LinearGradientBrush foreBrush(Rect(0, 0, 1, rect.GetBottom() + rect.GetTop()), _c1, _c2, LinearGradientModeVertical);
    g.FillPath(&foreBrush, &path);

	// pen1のサイズでrectを取得
	if (_borderSize1 > 0) path.Widen(&pen1);
	path.GetBounds(&rect);
	if (_borderSize2 > F(0)) {
		rect.Height = rect.Height + _borderSize2 * 2;
	}
	g.Flush();
	SAFE_DELETE(ff);

	if (false) {
		UINT num;        // number of image encoders
		UINT size;       // size, in bytes, of the image encoder array
		ImageCodecInfo* pImageCodecInfo;
		GetImageEncodersSize(&num, &size);
		pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
		GetImageEncoders(num, size, pImageCodecInfo);
		for (int i = 0; i < num ; i ++) {
			if (!wcscmp(pImageCodecInfo[i].MimeType, L"image/png")) {
				bitmap.Save(L"test.png", &pImageCodecInfo[i].Clsid);
				break;
			}
		}
		free(pImageCodecInfo);
	}
}
