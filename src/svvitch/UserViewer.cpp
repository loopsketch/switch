#ifdef USE_OPENNI

#include "UserViewer.h"
#include <Poco/format.h>


UserViewer::UserViewer(Renderer& renderer, xn::UserGenerator& userGenerator, xn::DepthGenerator& depthGenerator, XnUserID id):
	_log(Poco::Logger::get("")), _renderer(renderer), _userGenerator(userGenerator), _depthGenerator(depthGenerator), _id(id), _height(-1)
{
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
	_posR.clear();
	_posP.clear();
	XnStatus ret = XN_STATUS_OK;
	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		XnSkeletonJointPosition jp;
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_HEAD, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_NECK, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_TORSO, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_WAIST, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);

		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_COLLAR, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_SHOULDER, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_ELBOW, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_WRIST, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_HAND, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_FINGERTIP, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);

		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_COLLAR, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_SHOULDER, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_ELBOW, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_WRIST, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_HAND, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_FINGERTIP, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);

		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_HIP, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_KNEE, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_ANKLE, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_FOOT, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);

		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_HIP, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_KNEE, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_ANKLE, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_FOOT, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);

		//XN_SKEL_HEAD			= 1,
		//XN_SKEL_NECK			= 2,
		//XN_SKEL_TORSO			= 3,
		//XN_SKEL_WAIST			= 4,

		//XN_SKEL_LEFT_COLLAR		= 5,
		//XN_SKEL_LEFT_SHOULDER	= 6,
		//XN_SKEL_LEFT_ELBOW		= 7,
		//XN_SKEL_LEFT_WRIST		= 8,
		//XN_SKEL_LEFT_HAND		= 9,
		//XN_SKEL_LEFT_FINGERTIP	=10,

		//XN_SKEL_RIGHT_COLLAR	=11,
		//XN_SKEL_RIGHT_SHOULDER	=12,
		//XN_SKEL_RIGHT_ELBOW		=13,
		//XN_SKEL_RIGHT_WRIST		=14,
		//XN_SKEL_RIGHT_HAND		=15,
		//XN_SKEL_RIGHT_FINGERTIP	=16,

		//XN_SKEL_LEFT_HIP		=17,
		//XN_SKEL_LEFT_KNEE		=18,
		//XN_SKEL_LEFT_ANKLE		=19,
		//XN_SKEL_LEFT_FOOT		=20,

		//XN_SKEL_RIGHT_HIP		=21,
		//XN_SKEL_RIGHT_KNEE		=22,
		//XN_SKEL_RIGHT_ANKLE		=23,
		//XN_SKEL_RIGHT_FOOT		=24	
	}

	if (!_posP.empty()) {
		_depthGenerator.ConvertRealWorldToProjective(_posP.size(), &(_posP.front()), &(_posP.front()));
	}
}

void UserViewer::draw() {
	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		if (!_posP.empty()) {
			DWORD col1 = 0xffff0000;
			DWORD col2 = 0xff00ff00;
			for (int i = 0; i < 3; ++i) {
				_renderer.drawLine(_posP[i].X, _posP[i].Y, col2, _posP[i + 1].X, _posP[i + 1].Y, col2);
				_renderer.drawTexture(_posP[i].X - 2, _posP[i].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
				_renderer.drawTexture(_posP[i + 1].X - 2, _posP[i + 1].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
			}
			col1 = 0xffffff00;
			col2 = 0xffffff00;
			for (int i = 4; i < 9; ++i) {
				_renderer.drawLine(_posP[i].X, _posP[i].Y, col2, _posP[i + 1].X, _posP[i + 1].Y, col2);
				_renderer.drawTexture(_posP[i].X - 2, _posP[i].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
				_renderer.drawTexture(_posP[i + 1].X - 2, _posP[i + 1].Y - 2, 4, 4, NULL, 0, col1, col1, col1, col1);
			}
			//for (int i = 10; i < 15; ++i) {
			//	_renderer.drawLine(_posP[i].X, _posP[i].Y, col, _posP[i + 1].X, _posP[i + 1].Y, col);
			//}
			//for (int i = 16; i < 19; ++i) {
			//	_renderer.drawLine(_posP[i].X, _posP[i].Y, col, _posP[i + 1].X, _posP[i + 1].Y, col);
			//}
			//for (int i = 20; i < 23; ++i) {
			//	_renderer.drawLine(_posP[i].X, _posP[i].Y, col, _posP[i + 1].X, _posP[i + 1].Y, col);
			//}
		}
	}
}
#endif
