#pragma once

#include "Scene.h"
#include <XnCppWrapper.h>
#include <Poco/Mutex.h>
#include "FPSCounter.h"

#pragma comment(lib, "openNI.lib")

/**
 * �������o�V�[���N���X.
 * �������o�̋@�\��񋟂��܂�
 */
class OpenNIScene: public Scene
{
private:
	xn::Context _context;
	xn::ImageGenerator _image;
	xn::ImageMetaData _metaData;
	LPDIRECT3DTEXTURE9 _cameraImage;
	LPDIRECT3DSURFACE9 _surface;

public:
	OpenNIScene(Renderer& renderer);

	virtual ~OpenNIScene();

	virtual bool initialize();

	virtual void process();

	virtual void draw1();

	virtual void draw2();
};

typedef OpenNIScene* OpenNIScenePtr;