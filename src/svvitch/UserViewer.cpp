#ifdef USE_OPENNI

#include "UserViewer.h"


UserViewer::UserViewer(Renderer& renderer, xn::UserGenerator& userGenerator, xn::DepthGenerator& depthGenerator, XnUserID id):
	_renderer(renderer), _userGenerator(userGenerator), _depthGenerator(depthGenerator), _id(id)
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

void UserViewer::process() {
	_posR.clear();
	_posP.clear();
	XnStatus ret = XN_STATUS_OK;
	XnBoundingBox3D box;
	ret = _depthGenerator.GetUserPositionCap().GetUserPosition(_id, box);
	if (ret == XN_STATUS_OK) {
		XnPoint3D top = box.RightTopFar;
		top.X = box.LeftBottomNear.X;
		top.Z = box.LeftBottomNear.Z;
		_height = distance3D(box.LeftBottomNear, top);
		_posP.push_back(box.RightTopFar);
		_posP.push_back(box.LeftBottomNear);
	}

	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		XnSkeletonJointPosition jp;
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_HEAD, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_LEFT_HAND, jp);
		_posR.push_back(jp);
		_posP.push_back(jp.position);
		skeleton.GetSkeletonJointPosition(_id, XN_SKEL_RIGHT_HAND, jp);
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
	if (!_posP.empty()) {
		DWORD col = 0xccccffcc;
		_renderer.drawLine(_posP[0].X, _posP[0].Y, col, _posP[0].X, _posP[1].Y, col);
		_renderer.drawLine(_posP[0].X, _posP[1].Y, col, _posP[1].X, _posP[1].Y, col);
		_renderer.drawLine(_posP[1].X, _posP[1].Y, col, _posP[1].X, _posP[0].Y, col);
		_renderer.drawLine(_posP[1].X, _posP[0].Y, col, _posP[0].X, _posP[0].Y, col);
		_renderer.drawFontTextureText(_posP[0].X, _posP[0].Y, 10, 10, 0xccff3333, Poco::format("HEIGHT %0.1hfcm", _height / 10));
	}

	xn::SkeletonCapability& skeleton = _userGenerator.GetSkeletonCap();
	if (skeleton.IsTracking(_id)) {
		if (!_posP.empty()) {
			_renderer.drawFontTextureText(_posP[2].X - 30, _posP[2].Y - 5, 10, 10, 0xccff3333, "[HEAD]");
			_renderer.drawFontTextureText(_posP[3].X - 40, _posP[3].Y - 5, 10, 10, 0xccff3333, "[L-HAND]");
			_renderer.drawFontTextureText(_posP[4].X - 40, _posP[4].Y - 5, 10, 10, 0xccff3333, "[R-HAND]");
		}
	}
}
#endif
