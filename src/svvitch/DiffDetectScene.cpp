#include "DiffDetectScene.h"
#include <Poco/Util/XMLConfiguration.h>
#include <Poco/UnicodeConverter.h>
#include <Poco/FileStream.h>
#include <Poco/Buffer.h>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>

#include "MainScene.h"
#include "CaptureScene.h"
#include "Utils.h"


DiffDetectScene::DiffDetectScene(Renderer& renderer): Scene(renderer),
	_w(320), _h(240), _frame(0), _subtract(0), _frame1(NULL), _frame2(NULL), _result(NULL), _gray(NULL)
{
}

DiffDetectScene::~DiffDetectScene() {
	//_worker = NULL;
	//_thread.join();

	SAFE_RELEASE(_frame1);
	SAFE_RELEASE(_frame2);
	SAFE_RELEASE(_result);
	_log.information("*release diff-detect-scene");
}

bool DiffDetectScene::initialize() {
	_log.information("*initialize diff-detect-scene");

	try {
		Poco::Util::XMLConfiguration* xml = new Poco::Util::XMLConfiguration("capture-config.xml");
		if (xml) {
			_w = xml->getInt("device[@w]", 640);
			_h = xml->getInt("device[@h]", 480);
			if (xml->hasProperty("diff")) {
				_previewX = xml->getInt("diff.preview.x", 0);
				_previewY = xml->getInt("diff.preview.y", 0);
				_previewW = xml->getInt("diff.preview.width", 320);
				_previewH = xml->getInt("diff.preview.height", 240);
				_subtract = xml->getDouble("diff.subtract", 0.5);
			}
			xml->release();
		}
	} catch (Poco::Exception& ex) {
		_log.warning(ex.displayText());
	}

	_frame1 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_frame2 = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_result = _renderer.createRenderTarget(_w, _h, D3DFMT_A8R8G8B8);
	_renderer.colorFill(_result, 0x00000000);
	_fx = _renderer.createEffect("fx/diff.fx");
	if (_fx) {
		HRESULT hr = _fx->SetTechnique("diffTechnique");
		if FAILED(hr) _log.warning("failed not set technique diffTechnique");
		hr = _fx->SetFloat( "subtract", F(_subtract));
		if FAILED(hr) _log.warning("failed not set subtract");
		hr = _fx->SetTexture("texture1", _frame1);
		if FAILED(hr) _log.warning("failed not set texture1");
		hr = _fx->SetTexture("texture2", _frame2);
		if FAILED(hr) _log.warning("failed not set texture2");
	}
	//_worker = this;
	//_thread.start(*_worker);

	return true;
}


void DiffDetectScene::process() {
	CaptureScenePtr scene = dynamic_cast<CaptureScenePtr>(_renderer.getScene("capture"));
	if (scene) {
		_renderer.copyTexture(_frame1, _frame2);
		_renderer.copyTexture(scene->getCameraImage(), _frame1);
	}
	_frame++;
}

void DiffDetectScene::draw1() {
	drawDiff();
}

void DiffDetectScene::drawDiff() {
	if (_result) {
		D3DSURFACE_DESC desc;
		HRESULT hr = _result->GetLevelDesc(0, &desc);
		LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
		LPDIRECT3DSURFACE9 orgRT;
		hr = device->GetRenderTarget(0, &orgRT);
		LPDIRECT3DSURFACE9 surface;
		_result->GetSurfaceLevel(0, &surface);
		hr = device->SetRenderTarget(0, surface);
		//device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if (_fx) {
			_fx->Begin(NULL, 0);
			_fx->BeginPass(0);
			DWORD col = 0xffffffff;
			VERTEX dst[] =
			{
				{F(0      - 0.5), F(0      - 0.5), 0.0f, 1.0f, col, 0, 0},
				{F(0 + _w - 0.5), F(0      - 0.5), 0.0f, 1.0f, col, 1, 0},
				{F(0      - 0.5), F(0 + _h - 0.5), 0.0f, 1.0f, col, 0, 1},
				{F(0 + _w - 0.5), F(0 + _h - 0.5), 0.0f, 1.0f, col, 1, 1}
			};
			//device->SetTexture(0, _result);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dst, VERTEX_SIZE);
			//device->SetTexture(0, NULL);
			_fx->EndPass();
			_fx->End();
		}
		SAFE_RELEASE(surface);

		hr = device->SetRenderTarget(0, orgRT);
		SAFE_RELEASE(orgRT);
	}
}

void DiffDetectScene::draw2() {
	Uint32 fps = _fpsCounter.getFPS();
	if (_visible) {
		if (_result) {
			LPDIRECT3DDEVICE9 device = _renderer.get3DDevice();
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			DWORD col = 0xffffffff;
			_renderer.drawTexture(_previewX, _previewY, _previewW, _previewH, _result, 0, col, col, col, col);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			_renderer.drawFontTextureText(_previewX, _previewY, 10, 10, 0xccff3333, "diff detect");
		} else {
			_renderer.drawFontTextureText(_previewX, _previewY, 10, 10, 0xccff3333, "NO SIGNAL");
		}
	}
}


void DiffDetectScene::run() {
}
