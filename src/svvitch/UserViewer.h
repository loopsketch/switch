#pragma once

#include <XnCppWrapper.h>
#include <vector>
#include <Poco/Logger.h>
#include "Renderer.h"

using std::vector;


/**
 * ÉÜÅ[ÉUÉrÉÖÉè.
 */
class UserViewer
{
private:
	Poco::Logger& _log;
	Renderer& _renderer;
	xn::DepthGenerator& _depthGenerator;
	xn::UserGenerator& _userGenerator;
	XnUserID _id;
	int _height;
	XnSkeletonJointPosition* _posR;
	XnPoint3D* _posP;
	//map<XnSkeletonJoint, XnSkeletonJointPosition> _pos;

	float distance2D(const XnPoint3D& p1, const XnPoint3D& p2);
	float distance3D(const XnPoint3D& p1, const XnPoint3D& p2);

public:
	UserViewer(Renderer& renderer, xn::UserGenerator& userGenerator, xn::DepthGenerator& depthGenerator, XnUserID id);
	~UserViewer();

	void setHeight(int h);

	void process();

	void draw();
};

typedef UserViewer* UserViewerPtr;
