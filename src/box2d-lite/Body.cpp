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

#include "Body.h"

#include <algorithm>
#include <cmath>

namespace kti_b2l {
namespace {
float polygonAreaAndCentroid(
	const std::vector<Vec2>& vertices,
	Vec2& outCentroid
) {
	float signedArea2 = 0.0f;
	Vec2 centroidAccum(0.0f, 0.0f);
	int const count = static_cast<int>(vertices.size());
	for (int i = 0; i < count; ++i) {
		Vec2 const& a = vertices[i];
		Vec2 const& b = vertices[(i + 1) % count];
		float const cross = Cross(a, b);
		signedArea2 += cross;
		centroidAccum += cross * (a + b);
	}
	if (Abs(signedArea2) < 1.0e-6f) {
		outCentroid.Set(0.0f, 0.0f);
		return 0.0f;
	}
	float const area = 0.5f * signedArea2;
	outCentroid = (1.0f / (3.0f * signedArea2)) * centroidAccum;
	return area;
}

float polygonInertiaAboutOrigin(const std::vector<Vec2>& vertices, float density) {
	float inertia = 0.0f;
	int const count = static_cast<int>(vertices.size());
	for (int i = 0; i < count; ++i) {
		Vec2 const& p1 = vertices[i];
		Vec2 const& p2 = vertices[(i + 1) % count];
		float const cross = Cross(p1, p2);
		float const intx2 = Dot(p1, p1) + Dot(p1, p2) + Dot(p2, p2);
		inertia += cross * intx2;
	}
	return density * Abs(inertia) / 12.0f;
}
} // namespace

Body::Body()
{
	position.Set(0.0f, 0.0f);
	rotation = 0.0f;
	velocity.Set(0.0f, 0.0f);
	angularVelocity = 0.0f;
	force.Set(0.0f, 0.0f);
	torque = 0.0f;
	friction = 0.2f;
	centroid.Set(0.0f, 0.0f);
	vertices = {
		Vec2(-0.5f, -0.5f),
		Vec2(0.5f, -0.5f),
		Vec2(0.5f, 0.5f),
		Vec2(-0.5f, 0.5f),
	};
	normals.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		Vec2 const& a = vertices[i];
		Vec2 const& b = vertices[(i + 1) % vertices.size()];
		Vec2 edge = b - a;
		float const len = edge.Length();
		if (len > 1.0e-6f) {
			normals[i] = (1.0f / len) * Vec2(edge.y, -edge.x);
		} else {
			normals[i].Set(0.0f, 0.0f);
		}
	}
	mass = FLT_MAX;
	invMass = 0.0f;
	I = FLT_MAX;
	invI = 0.0f;
}

void Body::Set(const Vec2* verts, int count, float m)
{
	assert(verts);
	assert(count >= 3);
	assert(count <= kMaxPolygonVertices);

	position.Set(0.0f, 0.0f);
	rotation = 0.0f;
	velocity.Set(0.0f, 0.0f);
	angularVelocity = 0.0f;
	force.Set(0.0f, 0.0f);
	torque = 0.0f;
	friction = 0.2f;
	centroid.Set(0.0f, 0.0f);

	vertices.assign(verts, verts + count);
	Vec2 computedCentroid(0.0f, 0.0f);
	float const signedArea = polygonAreaAndCentroid(vertices, computedCentroid);
	assert(Abs(signedArea) > 1.0e-6f);
	if (signedArea < 0.0f) {
		std::reverse(vertices.begin(), vertices.end());
		polygonAreaAndCentroid(vertices, computedCentroid);
	}
	centroid = computedCentroid;
	for (Vec2& v : vertices) {
		v -= centroid;
	}
	normals.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		Vec2 const& a = vertices[i];
		Vec2 const& b = vertices[(i + 1) % vertices.size()];
		Vec2 edge = b - a;
		float const len = edge.Length();
		assert(len > 1.0e-6f);
		normals[i] = (1.0f / len) * Vec2(edge.y, -edge.x);
	}

	float const area = Abs(signedArea);
	mass = m;

	if (mass < FLT_MAX)
	{
		invMass = 1.0f / mass;
		float const density = area > 1.0e-6f ? (mass / area) : 0.0f;
		I = polygonInertiaAboutOrigin(vertices, density);
		if (I <= 1.0e-6f) {
			I = 1.0e-6f;
		}
		invI = 1.0f / I;
	}
	else
	{
		invMass = 0.0f;
		I = FLT_MAX;
		invI = 0.0f;
	}
}

} // namespace kti_b2l
