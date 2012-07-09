#ifdef USE_OPENNI

#include "UserViewer.h"
#include <Poco/format.h>


UserViewer::UserViewer(Renderer& renderer, xn::UserGenerator& userGenerator, xn::DepthGenerator& depthGenerator, XnUserID id):
	_log(Poco::Logger::get("")), _renderer(renderer), _userGenerator(userGenerator), _depthGenerator(depthGenerator), _id(id), _height(-1)
{
	_posR = new XnSkeletonJointPosition[24];
	_posP = new XnPoint3D[24];
}

UserViewer::~UserViewer() {
}

float UserViewer::distance2D(const XnPoint3D& p1, const XnPoint3D& p2) {
	float x = p2.X - p1.X;
	float y = p2.Y - p1.Y;
	return sqrtf(x * x + y * y);
}

float UserViewer::distance3D(const XnPoint3D& p1, const XnPoint3D& p2) {
	float x = p2.X - p1.X;
	float y = p2.Y - p1.Y;
	float z = p2.Z - p1.X;
	return sqrtf(x * x + y * y + z * z);
}

void UserViewer::setHeight(int h) {
	if (_height < 0 || _height > h) {
		_height = h;
		_log.debug(Poco::format("high: %d", h));
	}
}
void UserViewer::process() {
	XnStatus ret = XN_STATUS_OK;
	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		#define GET_JOINT(i, joint) {skeleton.GetSkeletonJointPosition(_id, joint, _posR[i]); _posP[i] = _posR[i].position;}
		GET_JOINT( 0, XN_SKEL_HEAD) // 0
		GET_JOINT( 1, XN_SKEL_NECK)
		GET_JOINT( 2, XN_SKEL_TORSO)
		GET_JOINT( 3, XN_SKEL_WAIST) // x

		GET_JOINT( 4, XN_SKEL_LEFT_COLLAR) // x
		GET_JOINT( 5, XN_SKEL_LEFT_SHOULDER) // 5
		GET_JOINT( 6, XN_SKEL_LEFT_ELBOW)
		GET_JOINT( 7, XN_SKEL_LEFT_WRIST) // x
		GET_JOINT( 8, XN_SKEL_LEFT_HAND)
		GET_JOINT( 9, XN_SKEL_LEFT_FINGERTIP) // x

		GET_JOINT(10, XN_SKEL_RIGHT_COLLAR) // x
		GET_JOINT(11, XN_SKEL_RIGHT_SHOULDER) // 11
		GET_JOINT(12, XN_SKEL_RIGHT_ELBOW)
		GET_JOINT(13, XN_SKEL_RIGHT_WRIST) // x
		GET_JOINT(14, XN_SKEL_RIGHT_HAND)
		GET_JOINT(15, XN_SKEL_RIGHT_FINGERTIP) // x

		GET_JOINT(16, XN_SKEL_LEFT_HIP) // 16
		GET_JOINT(17, XN_SKEL_LEFT_KNEE)
		GET_JOINT(18, XN_SKEL_LEFT_ANKLE)
		GET_JOINT(19, XN_SKEL_LEFT_FOOT)

		GET_JOINT(20, XN_SKEL_RIGHT_HIP) // 20
		GET_JOINT(21, XN_SKEL_RIGHT_KNEE)
		GET_JOINT(22, XN_SKEL_RIGHT_ANKLE)
		GET_JOINT(23, XN_SKEL_RIGHT_FOOT)
	}

	_depthGenerator.ConvertRealWorldToProjective(24, _posP, _posP);
}

void UserViewer::draw() {
	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		DWORD col1 = 0xffff0000;
		DWORD col2 = 0xff00ff00;
		for (int i = 0; i < 2; ++i) {
			_renderer.drawLine(_posP[i].X, _posP[i].Y, col2, _posP[i + 1].X, _posP[i + 1].Y, col2);
			_renderer.drawTexture(_posP[i].X - 2, _posP[i].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
			_renderer.drawTexture(_posP[i + 1].X - 2, _posP[i + 1].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		}
		col1 = 0xffffff00;
		col2 = 0xffffff00;
		_renderer.drawLine(_posP[5].X, _posP[5].Y, col2, _posP[6].X, _posP[6].Y, col2);
		_renderer.drawTexture(_posP[5].X - 2, _posP[5].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawTexture(_posP[6].X - 2, _posP[6].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawLine(_posP[6].X, _posP[6].Y, col2, _posP[8].X, _posP[8].Y, col2);
		_renderer.drawTexture(_posP[8].X - 2, _posP[8].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);

		_renderer.drawLine(_posP[11].X, _posP[11].Y, col2, _posP[12].X, _posP[12].Y, col2);
		_renderer.drawTexture(_posP[11].X - 2, _posP[11].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawTexture(_posP[12].X - 2, _posP[12].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawLine(_posP[12].X, _posP[12].Y, col2, _posP[14].X, _posP[14].Y, col2);
		_renderer.drawTexture(_posP[14].X - 2, _posP[14].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);

		col1 = 0xffff6600;
		col2 = 0xffff6600;
		_renderer.drawLine(_posP[16].X, _posP[16].Y, col2, _posP[17].X, _posP[17].Y, col2);
		_renderer.drawTexture(_posP[16].X - 2, _posP[16].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawTexture(_posP[17].X - 2, _posP[17].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawLine(_posP[17].X, _posP[17].Y, col2, _posP[19].X, _posP[19].Y, col2);
		_renderer.drawTexture(_posP[19].X - 2, _posP[19].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);

		_renderer.drawLine(_posP[20].X, _posP[20].Y, col2, _posP[21].X, _posP[21].Y, col2);
		_renderer.drawTexture(_posP[20].X - 2, _posP[20].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawTexture(_posP[21].X - 2, _posP[21].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
		_renderer.drawLine(_posP[21].X, _posP[21].Y, col2, _posP[23].X, _posP[23].Y, col2);
		_renderer.drawTexture(_posP[23].X - 2, _posP[23].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);

		//string s = Poco::format("joint %?u", _posP.size());
		//_renderer.drawFontTextureText(0, 10, 10, 10, 0xccff3333, s);
	}
}
#endif
