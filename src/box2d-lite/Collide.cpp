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

#include "ModTuning.h"
#include "Arbiter.h"
#include "Body.h"

namespace kti_b2l {
namespace {
constexpr char NO_EDGE = 0;

struct ClipVertex
{
	ClipVertex() { fp.value = 0; }
	Vec2 v;
	FeaturePair fp;
};

void Flip(FeaturePair& fp)
{
	Swap(fp.e.inEdge1, fp.e.inEdge2);
	Swap(fp.e.outEdge1, fp.e.outEdge2);
}

std::vector<Vec2> worldVertices(Body const* body) {
	Mat22 const rot(body->rotation);
	std::vector<Vec2> out;
	out.reserve(body->vertices.size());
	for (Vec2 const& v : body->vertices) {
		out.push_back(body->position + rot * v);
	}
	return out;
}

void projectPolygon(std::vector<Vec2> const& vertices, Vec2 const& axis, float& outMin, float& outMax) {
	outMin = outMax = Dot(vertices[0], axis);
	for (size_t i = 1; i < vertices.size(); ++i) {
		float const p = Dot(vertices[i], axis);
		if (p < outMin) outMin = p;
		if (p > outMax) outMax = p;
	}
}

float findAxisLeastPenetration(
	Body const* reference,
	std::vector<Vec2> const& referenceVerts,
	[[maybe_unused]] Body const* incident,
	std::vector<Vec2> const& incidentVerts,
	int& outRefEdge
) {
	Mat22 const referenceRot(reference->rotation);
	float bestSeparation = -FLT_MAX;
	int bestEdge = 0;
	for (size_t i = 0; i < reference->normals.size(); ++i) {
		Vec2 const worldNormal = referenceRot * reference->normals[i];
		float refMin = 0.0f;
		float refMax = 0.0f;
		float incMin = 0.0f;
		float incMax = 0.0f;
		projectPolygon(referenceVerts, worldNormal, refMin, refMax);
		projectPolygon(incidentVerts, worldNormal, incMin, incMax);
		float const separation = incMin - refMax;
		if (separation > bestSeparation) {
			bestSeparation = separation;
			bestEdge = static_cast<int>(i);
		}
	}
	outRefEdge = bestEdge;
	return bestSeparation;
}

void computeIncidentEdge(
	ClipVertex outIncidentEdge[2],
	Body const* incident,
	std::vector<Vec2> const& incidentVerts,
	Vec2 const& referenceNormal
) {
	Mat22 const incidentRot(incident->rotation);
	float minDot = FLT_MAX;
	int incidentEdge = 0;
	for (size_t i = 0; i < incident->normals.size(); ++i) {
		Vec2 const worldNormal = incidentRot * incident->normals[i];
		float const d = Dot(worldNormal, referenceNormal);
		if (d < minDot) {
			minDot = d;
			incidentEdge = static_cast<int>(i);
		}
	}

	int const next = (incidentEdge + 1) % static_cast<int>(incidentVerts.size());
	outIncidentEdge[0].v = incidentVerts[incidentEdge];
	outIncidentEdge[1].v = incidentVerts[next];

	char const edgeA = static_cast<char>(incidentEdge + 1);
	char const edgeB = static_cast<char>(next + 1);
	outIncidentEdge[0].fp.e.inEdge2 = edgeA;
	outIncidentEdge[0].fp.e.outEdge2 = edgeB;
	outIncidentEdge[1].fp.e.inEdge2 = edgeB;
	outIncidentEdge[1].fp.e.outEdge2 = edgeA;
}

int ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
					  const Vec2& normal, float offset, char clipEdge)
{
	// Start with no output points
	int numOut = 0;

	// Calculate the distance of end points to the line
	float distance0 = Dot(normal, vIn[0].v) - offset;
	float distance1 = Dot(normal, vIn[1].v) - offset;

	// If the points are behind the plane
	if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
	if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];

	// If the points are on different sides of the plane
	if (distance0 * distance1 < 0.0f)
	{
		// Find intersection point of edge and plane
		float interp = distance0 / (distance0 - distance1);
		vOut[numOut].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);
		if (distance0 > 0.0f)
		{
			vOut[numOut].fp = vIn[0].fp;
			vOut[numOut].fp.e.inEdge1 = clipEdge;
			vOut[numOut].fp.e.inEdge2 = NO_EDGE;
		}
		else
		{
			vOut[numOut].fp = vIn[1].fp;
			vOut[numOut].fp.e.outEdge1 = clipEdge;
			vOut[numOut].fp.e.outEdge2 = NO_EDGE;
		}
		++numOut;
	}

	return numOut;
}
} // namespace

// The normal points from A to B
int Collide(Contact* contacts, Body* bodyA, Body* bodyB)
{
	if (bodyA->vertices.size() < 3 || bodyB->vertices.size() < 3)
		return 0;

	std::vector<Vec2> const vertsA = worldVertices(bodyA);
	std::vector<Vec2> const vertsB = worldVertices(bodyB);

	int refEdgeA = 0;
	int refEdgeB = 0;
	float const separationA = findAxisLeastPenetration(bodyA, vertsA, bodyB, vertsB, refEdgeA);
	if (separationA > 0.0f)
		return 0;
	float const separationB = findAxisLeastPenetration(bodyB, vertsB, bodyA, vertsA, refEdgeB);
	if (separationB > 0.0f)
		return 0;

	Body* refBody = bodyA;
	Body* incBody = bodyB;
	std::vector<Vec2> const* refVerts = &vertsA;
	std::vector<Vec2> const* incVerts = &vertsB;
	int refEdge = refEdgeA;
	bool flip = false;
	if (separationB > separationA * kB2CollideReferenceEdgeRelativeTol + kB2CollideReferenceEdgeAbsoluteTol) {
		refBody = bodyB;
		incBody = bodyA;
		refVerts = &vertsB;
		incVerts = &vertsA;
		refEdge = refEdgeB;
		flip = true;
	}

	Mat22 const refRot(refBody->rotation);
	Vec2 const v1 = (*refVerts)[refEdge];
	Vec2 const v2 = (*refVerts)[(refEdge + 1) % static_cast<int>(refVerts->size())];
	Vec2 sideNormal = v2 - v1;
	float sideLen = sideNormal.Length();
	if (sideLen <= 1.0e-6f) {
		return 0;
	}
	sideNormal *= 1.0f / sideLen;
	Vec2 const refNormal = refRot * refBody->normals[refEdge];

	ClipVertex incidentEdge[2];
	computeIncidentEdge(incidentEdge, incBody, *incVerts, refNormal);

	float const negSide = -Dot(sideNormal, v1);
	float const posSide = Dot(sideNormal, v2);
	int const refCount = static_cast<int>(refVerts->size());
	char const negEdge = static_cast<char>(refEdge + 1);
	char const posEdge = static_cast<char>(((refEdge + 1) % refCount) + 1);

	ClipVertex clipPoints1[2];
	ClipVertex clipPoints2[2];
	int np;

	// Clip to reference edge side planes
	np = ClipSegmentToLine(clipPoints1, incidentEdge, -sideNormal, negSide, negEdge);

	if (np < 2)
		return 0;

	np = ClipSegmentToLine(clipPoints2, clipPoints1,  sideNormal, posSide, posEdge);

	if (np < 2)
		return 0;

	float const front = Dot(refNormal, v1);
	Vec2 const collisionNormal = flip ? (-1.0f * refNormal) : refNormal;
	int numContacts = 0;
	for (int i = 0; i < 2; ++i)
	{
		float separation = Dot(refNormal, clipPoints2[i].v) - front;

		if (separation <= 0)
		{
			contacts[numContacts].separation = separation;
			contacts[numContacts].normal = collisionNormal;
			contacts[numContacts].position = clipPoints2[i].v - separation * refNormal;
			contacts[numContacts].feature = clipPoints2[i].fp;
			contacts[numContacts].feature.e.inEdge1 = static_cast<char>(refEdge + 1);
			contacts[numContacts].feature.e.outEdge1 = static_cast<char>(((refEdge + 1) % refCount) + 1);
			if (flip)
				Flip(contacts[numContacts].feature);
			++numContacts;
		}
	}

	return numContacts;
}

} // namespace kti_b2l
