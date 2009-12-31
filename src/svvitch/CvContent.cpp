#include <Poco/DateTime.h>
#include <Poco/Timezone.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/Stringtokenizer.h>
#include <Poco/Util/XMLConfiguration.h>

#include "CvContent.h"

using Poco::Util::XMLConfiguration;


CvContent::CvContent(Renderer& renderer): Content(renderer), activeOpenDetectMovie(this, &CvContent::openDetectMovie),
	_normalItem(NULL), _normalMovie(NULL), _detectedItem(NULL), _detectedMovie(NULL),
	_fx(NULL), _small1(NULL), _small2(NULL), _diff(NULL), _diff2(NULL), _photo(NULL), 
	_detected(false), _doShutter(0), _viewPhoto(0), _finished(true), _playing(false), _statusFrame(0)
{
	initialize();
	_scene = (CaptureScenePtr)_renderer.getScene("capture");
}

CvContent::~CvContent() {
	initialize();
}

void CvContent::saveConfiguration() {
	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("cv-config.xml");
		if (xml) {
			xml->setInt("detect", _detectThreshold);
			xml->setInt("intervals[@diff]", _intervalDiff);
			xml->setInt("intervals[@small]", _intervalSmall);
			xml->setInt("clip[@x]", _clipX);
			xml->setInt("clip[@y]", _clipY);
			xml->setInt("clip[@w]", _clipW);
			xml->setInt("clip[@h]", _clipH);
			xml->save("cv-config.xml");
			xml->release();
			_log.information("cv parameters saved");
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}
}

void CvContent::initialize() {
	srand(time(NULL));
	close();
}

/** ファイルをオープンします */
bool CvContent::open(const MediaItemPtr media, const int offset) {
	initialize();

	int deviceW = 0;
	int deviceH = 0;
	try {
		XMLConfiguration* xml = new XMLConfiguration("cv-config.xml");
//		_deviceNo = xml->getInt("device[@no]", 0);
		deviceW = xml->getInt("device[@w]", 640);
		deviceH = xml->getInt("device[@h]", 480);
//		_deviceFPS = xml->getInt("device[@fps]", 60);
		_subtract = xml->getDouble("subtract", 0.5);
//		_samples = xml->getInt("samples", 4);
		_detectThreshold = xml->getInt("detect", 50);
		_intervalDiff = xml->getInt("intervals[@diff]", 60);
		_intervalSmall = xml->getInt("intervals[@small]", 12);
		_clipX = xml->getInt("clip[@x]", 0);
		_clipY = xml->getInt("clip[@y]", 0);
		_clipW = xml->getInt("clip[@w]", deviceW);
		_clipH = xml->getInt("clip[@h]", deviceH);
		_normalFile = xml->getString("normalMovie", "");
		string detectedFile = xml->getString("detectedMovie", "");
		Poco::StringTokenizer files(detectedFile, ",");
		for (Poco::StringTokenizer::Iterator it = files.begin(); it != files.end(); it++) {
			_detectFiles.push_back(*it);
		}
		_detectCount = xml->getInt("detectedMovie[@count]", 1);

		xml->release();
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	vector<MediaItemFile> file1;
	file1.push_back(MediaItemFile(MediaTypeMovie, _normalFile, ""));
	_normalItem = new MediaItem(MediaTypeMovie, "normal", "normal", 0, file1);
	_normalMovie = new FFMovieContent(_renderer);
	if (_normalMovie->open(_normalItem)) {
		_normalMovie->setPosition(config().stageRect.left, config().stageRect.top);
		_normalMovie->setBounds(config().stageRect.right, config().stageRect.bottom);
		_normalMovie->play();
	} else {
		SAFE_DELETE(_normalMovie);
	}

	_detectedMovie = new FFMovieContent(_renderer);
	_detectedMovie->setPosition(config().stageRect.left, config().stageRect.top);
	_detectedMovie->setBounds(config().stageRect.right, config().stageRect.bottom);
	activeOpenDetectMovie();

	std::wstring wfile;
	Poco::UnicodeConverter::toUTF16(string("subbg.fx"), wfile);
	LPD3DXBUFFER errors = NULL;
	HRESULT hr = D3DXCreateEffectFromFile(_renderer.get3DDevice(), wfile.c_str(), 0, 0, D3DXSHADER_DEBUG, 0, &_fx, &errors);
	if (errors) {
		std::vector<char> text(errors->GetBufferSize());
		memcpy(&text[0], errors->GetBufferPointer(), errors->GetBufferSize());
		text.push_back('\0');
		_log.warning(Poco::format("shader compile error: %s", string(&text[0])));
		SAFE_RELEASE(errors);
	} else if (FAILED(hr)) {
		_log.warning(Poco::format("failed shader: %s", string("")));
	}

	_small1 = _renderer.createRenderTarget(deviceW / 8, deviceH / 8, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_small1, 0);
	_small2 = _renderer.createRenderTarget(deviceW / 8, deviceH / 8, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_small2, 0);
	_diff = _renderer.createRenderTarget(deviceW / 8, deviceH / 8, D3DFMT_A8R8G8B8);
	_diff2 = _renderer.createLockableSurface(deviceW / 8, deviceH / 8, D3DFMT_A8R8G8B8);
	_photo = _renderer.createRenderTarget(deviceW, deviceH, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_photo, 0);

	set("alpha", 1.0f);
	_duration = media->duration() * 60 / 1000;
	_current = 0;
	_mediaID = media->id();
	return true;
}

void CvContent::openDetectMovie() {
	_log.information("*openDetectMovie");
	SAFE_DELETE(_detectedItem);
	if (!_detectFiles.empty()) {
		vector<MediaItemFile> files;
		files.push_back(MediaItemFile(MediaTypeMovie, _detectFiles.at(rand() % _detectFiles.size()), ""));
		_detectedItem = new MediaItem(MediaTypeMovie, "detected", "detected", 0, files);
		if (_detectedMovie->open(_detectedItem)) {
	//		_detectedMovie->setPosition(conf->stageRect.left, conf->stageRect.top);
	//		_detectedMovie->setBounds(conf->stageRect.right, conf->stageRect.bottom);
	//	} else {
	//		SAFE_DELETE(_detectedMovie);
		}
	}
}


/**
 * 再生
 */
void CvContent::play() {
	_playing = true;
	_playTimer.start();
}

/**
 * 停止
 */
void CvContent::stop() {
	_playing = false;
}

bool CvContent::useFastStop() {
	return true;
}

/**
 * 再生中かどうか
 */
const bool CvContent::playing() const {
	return _playing;
}

const bool CvContent::finished() {
	if (_duration > 0) {
		if (_detectedMovie && _detectedMovie->playing() && !_detectedMovie->finished()) {
			return false;
		}
		return _current >= _duration;
	}
	return false;
}

/** ファイルをクローズします */
void CvContent::close() {
	stop();
	_mediaID.clear();

	Poco::DateTime now;
	now.makeLocal(Poco::Timezone::tzd());
	std::wstring wfile;
	Poco::UnicodeConverter::toUTF16(Poco::format("photos/snap%02d%02d%02d%02d.png", now.day(), now.hour(), now.minute(), now.second()), wfile);
	HRESULT hr = D3DXSaveTextureToFile(wfile.c_str(), D3DXIFF_PNG, _photo, NULL);

	{
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		SAFE_RELEASE(_fx);
		SAFE_RELEASE(_small1);
		SAFE_RELEASE(_small2);
		SAFE_RELEASE(_diff);
		SAFE_RELEASE(_diff2);
		SAFE_RELEASE(_photo);
		SAFE_DELETE(_normalMovie);
		SAFE_DELETE(_normalItem);
		SAFE_DELETE(_detectedMovie);
		SAFE_DELETE(_detectedItem);
	}
}

void CvContent::process(const DWORD& frame) {
	if (_playing) {
		if (_keycode != 0) {
			switch (_keycode) {
				case 'P':
					_clipY-=1;
					break;
				case VK_OEM_PERIOD:
					_clipY+=1;
					break;
				case 'L':
					_clipX-=1;
					break;
				case VK_OEM_PLUS:
					_clipX+=1;
					break;

				case VK_OEM_4:
					_clipH-=1;
					break;
				case VK_OEM_102:
					_clipH+=1;
					break;
				case VK_OEM_1:
					_clipW-=1;
					break;
				case VK_OEM_6:
					_clipW+=1;
					break;

				case '8':
					_detectThreshold--;
					break;
				case '9':
					_detectThreshold++;
					break;

				case '0':
					_intervalDiff--;
					break;
				case VK_OEM_MINUS:
					_intervalDiff++;
					break;

				case VK_OEM_7:
					_intervalSmall--;
					break;
				case VK_OEM_5:
					_intervalSmall++;
					break;

				case 'S':
					saveConfiguration();
					_statusFrame = frame;
					_status = "PARAMETER SAVED.";
					break;

				default:
					_log.information(Poco::format("key: %?x", _keycode));
			}
		}

//		if (get("prepare") != "true") {
//		}
		if (_normalMovie) {
			_normalMovie->process(frame);
			if (_normalMovie->playing() && _normalMovie->finished()) {
				_normalMovie->seek(0);
				_normalMovie->play();
			}
		}
		if (_detectedMovie) {
			_detectedMovie->process(frame);
			if (!_detectedMovie->opened().empty() && !_detected && _diffCount > _detectThreshold) {
				if (_viewPhoto == 0 || (frame - _viewPhoto) > 500) {
					if (_detectCount > 0) {
						if (_normalMovie) _normalMovie->stop();
						_detectedMovie->play();
						_detected = true;
						_doShutter = frame;
					} else {
						// 検出回数上限
						_current = _duration;
					}
				}
			}
			if (_detectedMovie->playing() && _detectedMovie->finished()) {
				_detectedMovie->close();
//				_detectedMovie->seek(0);
				if (_normalMovie) _normalMovie->play();
				_detectCount--;
				if (_detectCount > 0) activeOpenDetectMovie();
				_detected = false;
				_viewPhoto = frame;
			}
		}
		_current++;
	}
}

void CvContent::draw(const DWORD& frame) {
	if (!_mediaID.empty() && _playing) {
		Poco::ScopedLock<Poco::FastMutex> lock(_lock);
		LPDIRECT3DTEXTURE9 cameraImage = _scene->getCameraImage();
		DWORD col = 0xffffffff;
		_renderer.drawTexture(640, 0, 640, 480, cameraImage, 0, col, col, col, col);
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		if (_fx) {
			LPDIRECT3DSURFACE9 orgRT;
			HRESULT hr = device->GetRenderTarget(0, &orgRT);

			// 単純化
			col = 0xffffffff;
			D3DSURFACE_DESC desc;
			hr = _small1->GetLevelDesc(0, &desc);
			VERTEX dst[] =
			{
				{F(0              - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 0, 0},
				{F(0 + desc.Width - 0.5), F(0               - 0.5), 0.0f, 1.0f, col, 1, 0},
				{F(0              - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 0, 1},
				{F(0 + desc.Width - 0.5), F(0 + desc.Height - 0.5), 0.0f, 1.0f, col, 1, 1}
			};
			LPDIRECT3DSURFACE9 surface;
			if ((frame % _intervalDiff) == 0) {
				_small1->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_renderer.drawTexture(0, 0, desc.Width, desc.Height, _clipX, _clipY, _clipW, _clipH, cameraImage, col, col, col, col);
				SAFE_RELEASE(surface);
			}
			if ((frame % _intervalSmall) == 0) {
				_small2->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_renderer.drawTexture(0, 0, desc.Width, desc.Height, _clipX, _clipY, _clipW, _clipH, cameraImage, col, col, col, col);
				SAFE_RELEASE(surface);

				// 差分
				_diff->GetSurfaceLevel(0, &surface);
				hr = device->SetRenderTarget(0, surface);
				_fx->SetTechnique("BasicTech");
				_fx->SetFloat("subtract", _subtract);
				_fx->SetTexture("bgTex", _small1);
				_fx->SetTexture("currentTex", _small2);
				hr = _fx->Begin(0, 0);
				if (SUCCEEDED(hr)) {
					_fx->BeginPass(1);
					device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
					_fx->EndPass();
					_fx->End();
				}
				SAFE_RELEASE(surface);
				if (_renderer.getRenderTargetData(_diff, _diff2)) {
					D3DSURFACE_DESC desc;
					_diff2->GetDesc(&desc);
					D3DLOCKED_RECT lockedRect;
					hr = _diff2->LockRect(&lockedRect, NULL, 0);
					if (SUCCEEDED(hr)) {
						_diffCount = 0;
						DWORD* buf = (DWORD*)lockedRect.pBits;
						for (int i = 0; i < lockedRect.Pitch / 4 * desc.Height; i++) {
							if (buf[i] == 0xffffffff) _diffCount++;
						}
						_diff2->UnlockRect();
						if (desc.Width * desc.Height <= _diffCount) _diffCount = 0;
					}
				}
			}
			hr = device->SetRenderTarget(0, orgRT);
			SAFE_RELEASE(orgRT);
		}

		if (_doShutter > 0 && (frame - _doShutter) > 60) {
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			LPDIRECT3DSURFACE9 orgRT;
			HRESULT hr = device->GetRenderTarget(0, &orgRT);

			D3DSURFACE_DESC desc;
			hr = _photo->GetLevelDesc(0, &desc);
			LPDIRECT3DSURFACE9 surface;
			_photo->GetSurfaceLevel(0, &surface);
			hr = device->SetRenderTarget(0, surface);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(0, 0, desc.Width, desc.Height, cameraImage, 0, col, col, col, col);
			SAFE_RELEASE(surface);

			hr = device->SetRenderTarget(0, orgRT);
			SAFE_RELEASE(orgRT);
			_doShutter = 0;
		}

		if (_detected) {
			if (_detectedMovie) _detectedMovie->draw(frame);
		} else {
			if (_normalMovie) _normalMovie->draw(frame);
		}

		int x = config().stageRect.left;
		int y = config().stageRect.top;
		int w = config().stageRect.right;
		int h = config().stageRect.bottom;
		if (_viewPhoto > 0 && (frame - _viewPhoto) > 100 && (frame - _viewPhoto) < 500) {
			float alpha = 1.0f;
			if ((frame - _viewPhoto) > 400) {
				alpha = F(500 - (frame - _viewPhoto)) / 100;
			}
			DWORD col = ((DWORD)(0x66 * alpha) << 24) | 0x000000;
			_renderer.drawTexture(x + 60, y + 20, w - 80, h - 20, NULL, 0, col, col, col, col);
			col = ((DWORD)(0xff * alpha) << 24) | 0xffffff;
			_renderer.drawTexture(x + 40, y + 10, w - 80, h - 20, NULL, 0, col, col, col, col);
			_renderer.drawTexture(x + 43, y + 13, w - 86, h - 26, _photo, 0, col, col, col, col);
		}

		// ステータス表示
		col = 0x66669966;
		_renderer.drawTexture(640 + _clipX, _clipY, _clipW, _clipH, NULL, 0, col, col, col, col);
		_renderer.drawFontTextureText(640 + _clipX, _clipY, 8, 10, 0xccffffff, Poco::format("(%d,%d)-%dx%d", _clipX, _clipY, _clipW, _clipH));
		col = 0xffffffff;
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		_renderer.drawTexture(320, 480, 320, 240, _diff, 0, col, col, col, col);
		_renderer.drawFontTextureText(320, 480, 12, 16, 0xccffffff, Poco::format("<%04d/%04d>", _diffCount, _detectThreshold));
		_renderer.drawTexture(640, 480, 320, 240, _small1, 0, col, col, col, col);
		float interval = _intervalDiff / 60.0f;
		_renderer.drawFontTextureText(640, 480, 12, 16, 0xccffffff, Poco::format("%d(%0.2hfs)", _intervalDiff, interval));
		_renderer.drawTexture(960, 480, 320, 240, _small2, 0, col, col, col, col);
		interval = _intervalSmall / 60.0f;
		_renderer.drawFontTextureText(960, 480, 12, 16, 0xccffffff, Poco::format("%d(%0.2hfs)", _intervalSmall, interval));
		_renderer.drawTexture(0, 480, 320, 240, _photo, 0, col, col, col, col);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		if (_statusFrame > 0 && (frame - _statusFrame) < 200) {
			_renderer.drawFontTextureText(640, config().mainRect.bottom - 32, 24, 32, 0xccff3333, _status);
		}
	}
}
