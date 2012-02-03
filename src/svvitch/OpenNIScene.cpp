#ifdef USE_OPENNI
#include "OpenNIScene.h"

OpenNIScene::OpenNIScene(Renderer& renderer): Scene(renderer)
{
	_log.information("OpenNI-scene");
}

OpenNIScene::~OpenNIScene() {
	_log.information("*uninitialize OpenNI-scene");
}

bool OpenNIScene::initialize() {
	XnStatus ret = XN_STATUS_OK;
	ret = _context.InitFromXmlFile("Config00.xml", NULL);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not initialize kinect");
		return false;
	}
	ret = _context.FindExistingNode(XN_NODE_TYPE_IMAGE, _image);
	if (ret != XN_STATUS_OK) {
		_log.warning("failed not found ImageGenerator");
		return false;
	}
	_cameraImage = _renderer.createRenderTarget(640, 480, D3DFMT_A8R8G8B8);
	_surface = _renderer.createLockableSurface(640, 480, D3DFMT_A8R8G8B8);

	_log.information("*initialize OpenNI-scene");
	return true;
}

void OpenNIScene::process() {
	_context.WaitOneUpdateAll(_image);
	_image.GetMetaData(_metaData);

	if (_renderer.getRenderTargetData(_cameraImage, _surface)) {
		D3DLOCKED_RECT lockedRect = {0};
		if (SUCCEEDED(_surface->LockRect(&lockedRect, NULL, 0))) {
			LPBYTE src = (LPBYTE)_metaData.RGB24Data();
			::memcpy(lockedRect.pBits, src, 640 * 480 * 3);
			_surface->UnlockRect();
		} else {
		_log.warning("failed LockRect");
		}
	} else {
		_log.warning("failed getRenderTargetData");
	}
	_metaData.RGB24Data();
}

void OpenNIScene::draw1() {
	DWORD col = 0xffffffff;
	_renderer.drawTexture(0, 0, 640, 480, _cameraImage, 0, col, col, col, col);
}

void OpenNIScene::draw2() {
}

#endif
