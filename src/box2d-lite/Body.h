/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability
* of this software for any purpose.
* It is provided "as is" without express or implied warranty.
*/

#ifndef BODY_H
#define BODY_H

#include "ModTuning.h"
#include "MathUtils.h"
#include <vector>

namespace kti_b2l {

struct Body
{
	static constexpr int kMaxPolygonVertices = kB2MaxPolygonVertices;

	Body();
	void Set(const Vec2* verts, int count, float m);

	void AddForce(const Vec2& f)
	{
		force += f;
	}

	Vec2 position;
	float rotation;

	Vec2 velocity;
	float angularVelocity;

	Vec2 force;
	float torque;

	std::vector<Vec2> vertices;
	std::vector<Vec2> normals;
	Vec2 centroid;

	float friction;
	float mass, invMass;
	float I, invI;
};

} // namespace kti_b2l

#endif
