#include "Text.h"

#include <Poco/FileStream.h>
#include <Poco/LineEndingConverter.h>
#include <Poco/RegularExpression.h>
#include <Poco/string.h>
#include <Poco/UnicodeConverter.h>

#include <gdiplus.h>

#include "Utils.h"

using namespace Gdiplus;


Text::Text(Renderer& renderer, float x, float y, float w, float h): Content(renderer, x, y, w, h), _texture(NULL), _referencedText(NULL)
{
//	PrivateFontCollection fc;
//	fc.AddFontFile(L"dfhsg9.ttc");
//	int num = 0;
//	fc.GetFamilies(16, _ff, &num);
//	if (num == 0) {
//		Font f(L"ＭＳ ゴシック", 16);
//		f.GetFamily(&_ff[0]);
//	}
	initialize();
}

Text::~Text() {
	initialize();
}

void Text::initialize() {
	Poco::ScopedLock<Poco::FastMutex> lock(_initializeLock);
	close();
	_dx = 0;
	_dy = 0;
}

bool Text::open(const MediaItemPtr media, const int offset) {
	initialize();
	Poco::RegularExpression re("^(sjis|shift_jis|shiftjis|ms932)$", Poco::RegularExpression::RE_CASELESS + Poco::RegularExpression::RE_EXTENDED);

	bool valid = false;
	if (offset != -1 && offset < media->fileCount()) {
		MediaItemFile mif =  media->files().at(offset);
		if (mif.type() == MediaTypeText) {
			if (!mif.file().empty()) {
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
							if (!text.empty()) text += "     ";
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
						_log.information(Poco::format("lines: %d", linenum));

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
			_font = mif.getProperty("font");
			if (_font.empty()) _font = config().textFont;
			_fontH = mif.getNumProperty("fh", config().textHeight);
			_x = mif.getNumProperty("x", 0);
			_y = mif.getNumProperty("y", 0);
			_w = mif.getNumProperty("w", config().stageRect.right);
			_h = mif.getNumProperty("h", config().stageRect.bottom);
			_cx = mif.getNumProperty("cx", _x);
			_cy = mif.getNumProperty("cy", _y);
			_cw = mif.getNumProperty("cw", _w);
			_ch = mif.getNumProperty("ch", _h);
			_dx = mif.getFloatProperty("dx", F(0));
			_dy = mif.getFloatProperty("dy", F(0));
			_align = mif.getProperty("align");
			_log.information(Poco::format("text: (%hf,%hf) %hfx%hf dx:%hf dy:%hf", _x, _y, _w, _h, _dx, _dy));
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

void Text::setReference(TextPtr text) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	_referencedText = text;
	_log.information("set reference text content");
}

void Text::play() {
	_playing = true;
}

void Text::stop() {
	_playing = false;
}

const bool Text::finished() {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_mediaID.empty() && _texture) {
		if (_dx != 0) {
			return true;
//			return _x < (_cx -_iw);
		}
	}
	return _playing;
}

/** ファイルをクローズします */
void Text::close() {
	_mediaID.clear();
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (_referencedText) {
		_referencedText = NULL;
		_texture = NULL;
	} else {
		SAFE_RELEASE(_texture);
	}
	_iw = 0;
	_ih = 0;
	_tw = 0;
	_th = 0;
}

/** 1フレームに1度だけ処理される */
void Text::process(const DWORD& frame) {
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
	if (!_mediaID.empty() && _texture) {
		if (_playing) {
			if (_dx != 0) {
				_x += _dx;
				if (_x < (_cx - _iw - config().stageRect.right)) _playing = false;
			}
		}
	}
}

/** 描画 */
void Text::draw(const DWORD& frame) {
	Poco::ScopedLock<Poco::FastMutex> lock(_lock);
	if (!_mediaID.empty() && _texture && _playing) {
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		float alpha = getF("alpha");
		DWORD col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
		int cw = config().splitSize.cx;
		int ch = config().splitSize.cy;

		switch (config().splitType) {
		case 1:
			{
				device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
				RECT scissorRect;
				device->GetScissorRect(&scissorRect);
				float x = _x + _ax;
				float y = _y;
				int dh = (640 / ch * cw);
				int ix = 0, sx = 0, sy = 0, dx = (int)x / dh * cw, dxx = fmod(x, cw), dy = ch * ((int)x / cw) % 640;
				int cww = 0;
				int chh = (ch > _ih)?_ih:ch;
				int clipX = _cx;
				while (dx < 1024) {
					RECT rect = {dx, dy, dx + cw, dy + chh};
					int cx = dx /cw * dh + dy / ch * cw;
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
					if ((sx + cw - dxx) >= _tw) {
						// ソースの横がはみ出る
						cww = _tw - sx - dxx;
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _texture, col, col, col, col);
//						_renderer.drawFontTextureText(dx + dxx, y + dy, 12, 12, 0xffffffff, Poco::format(">%d,%d", sx, sy));
						sx = 0;
						sy += _ih;
						if (sy < _th) {
							// srcを折り返してdstの残りに描画
							if (_th - sy < ch) chh = _th - sy;
							_renderer.drawTexture(dx + dxx + cww, y + dy, cw - cww, chh, sx, sy, cw - cww, chh, _texture, col, col, col, col);
//							_renderer.drawFontTextureText(dx + dxx + cww, y + dy, 12, 12, 0xffffffff, Poco::format("t%d,%d", sx, sy));
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
						_renderer.drawTexture(dx + dxx, y + dy, cww, chh, sx, sy, cww, chh, _texture, col, col, col, col);
//						_renderer.drawFontTextureText(dx + dxx, y + dy, 12, 12, 0xffffffff, Poco::format("%d,%d", sx, sy));
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
					if (dy >= 640) {
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
						if (tcw > 0 && tch > 0) _renderer.drawTexture(ox + dx, oy + dy, tcw, tch, tx, ty, tcw, tch, _texture, col, col, col, col);
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
				_renderer.drawTexture(_x, _y, _texture, col, col, col, col);
				_x+=_dx;
				if (_x < -_tw) _x = config().stageRect.right;
				_y+=_dy;
			}

		}
	} else {
		if (get("prepare") == "true") {
			int sy = getF("itemNo") * 20;
			_renderer.drawTexture(700, 600 + sy, 324, 20, _texture, 0xccffffff, 0xccffffff,0xccffffff, 0xccffffff);
		}
	}
}

void Text::drawTexture(string text) {
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
			tw = 1024;
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
				// D3DXSaveTextureToFile(L"test_text.png", D3DXIFF_PNG, texture, NULL);
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
			// D3DXSaveTextureToFile(L"test_text.png", D3DXIFF_PNG, texture, NULL);
		}
		{
			Poco::ScopedLock<Poco::FastMutex> lock(_lock);
			if (_texture) SAFE_RELEASE(_texture);
			_texture = texture;
			_iw = iw;
			_ih = ih;
			_tw = tw;
			_th = th;
		}
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

void Text::drawText(string text, Bitmap& bitmap, Rect& rect) {
	int x = 0;
	int y = 0;
	if (rect.GetRight() - rect.GetLeft() != 0 || rect.GetBottom() - rect.GetTop() != 0) {
		x = -rect.GetLeft();
	}

	std::wstring wtext;
	Poco::UnicodeConverter::toUTF16(text, wtext);

	Graphics g(&bitmap);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintAntiAlias);

	std::wstring wfontFamily;
	Poco::UnicodeConverter::toUTF16(_font, wfontFamily);
	Font f(wfontFamily.c_str(), _fontH);
	Gdiplus::FontFamily ff;
	f.GetFamily(&ff);
//	path.AddString(wtext.c_str(), wcslen(wtext.c_str()), &_ff[0], FontStyleRegular, 30, Point(x, y), StringFormat::GenericDefault());
	GraphicsPath path;

	path.AddString(wtext.c_str(), wcslen(wtext.c_str()), &ff, config().textStyle, _fontH, Point(x, y), StringFormat::GenericDefault());
	LinearGradientBrush foreBrush(Rect(0, 0, 1, _fontH), Color(255, 255, 255), Color(128, 128, 128), LinearGradientModeVertical);
	SolidBrush borderBrush1(Color(0, 224, 224, 224));
	SolidBrush borderBrush2(Color::Black);
	Pen pen1(&borderBrush1, 8);
	Pen pen2(&borderBrush2, 4);
	pen1.SetLineJoin(LineJoinRound);
	pen2.SetLineJoin(LineJoinRound);
	g.DrawPath(&pen1, &path);
	g.DrawPath(&pen2, &path);
    g.FillPath(&foreBrush, &path);

	// pen1のサイズでrectを取得
	path.Widen(&pen2);
	path.GetBounds(&rect);
	g.Flush();

//	{
//		UINT num;        // number of image encoders
//		UINT size;       // size, in bytes, of the image encoder array
//		ImageCodecInfo* pImageCodecInfo;
//		GetImageEncodersSize(&num, &size);
//		pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
//		GetImageEncoders(num, size, pImageCodecInfo);
//		for (int i = 0; i < num ; i ++) {
//			if (!wcscmp(pImageCodecInfo[i].MimeType, L"image/png")) {
//				bitmap.Save(L"test.png", &pImageCodecInfo[i].Clsid);
//				break;
//			}
//		}
//		free(pImageCodecInfo);
//	}
}
