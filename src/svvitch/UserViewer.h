#pragma once

#include <XnCppWrapper.h>
#include <vector>
#include "Renderer.h"

using std::vector;


/**
 * ƒ†[ƒUƒrƒ…ƒ.
 */
class UserViewer
{
private:
	Renderer& _renderer;
	xn::DepthGenerator& _depthGenerator;
	xn::UserGenerator& _userGenerator;
	XnUserID _id;
	float _height;
	vector<XnSkeletonJointPosition> _posR;
	vector<XnPoint3D> _posP;
	//map<XnSkeletonJoint, XnSkeletonJointPosition> _pos;

	float distance2D(const XnPoint3D& p1, const XnPoint3D& p2);
	float distance3D(const XnPoint3D& p1, const XnPoint3D& p2);

public:
	UserViewer(Renderer& renderer, xn::UserGenerator& userGenerator, xn::DepthGenerator& depthGenerator, XnUserID id);
	~UserViewer();

	void process();

	void draw();
};

typedef UserViewer* UserViewerPtr;
