#pragma once

#include "Maths.h"

#include <functional>

using namespace improbable::math;

namespace geometry
{
	//------------------------------------------
	struct Sphere
	{
		Sphere() : Origin(zero3<Vector3f>()) {}
		Sphere(TVector3fArg origin, float radius) : Origin(origin), Radius(radius) {}
		
		// todo: Vector4 implementation
		Vector3f Origin;
		float Radius;
	};
	//------------------------------------------
	struct Plane
	{
		Plane() : Normal(unitY3<Vector3f>()), DistanceToOrigin(0.0f) {}
		Plane(TVector3fArg n, TVector3fArg posOnPlane) : Normal(n), DistanceToOrigin(dot(posOnPlane, n)) {}
		Vector3f Normal;
		float DistanceToOrigin;
	};
	
	//------------------------------------------
	struct Aabb3
	{
		Aabb3(TVector3fArg lbb, TVector3fArg rtf) : LeftBottomBack(lbb), RightTopFront(rtf) {}

		Vector3f RightTopFront;
		Vector3f LeftBottomBack;
	};

	//------------------------------------------
	class CubeSphereIntersection
	{
	public:
		CubeSphereIntersection(const Aabb3& box, float radius);

		bool IntersectionAt(TVector3fArg spherePos) const;
		
		enum
		{
			SuperBox = 0,
			StretchX,
			StretchY,
			StretchZ,
			NumStretches
		};

		Plane Planes[NumStretches*6];
		Sphere Vertices[8];
	};
	
	inline bool sphereContains(const Sphere& sphere, TVector3fArg pos)
	{
		return sqrMag(sphere.Origin - pos) < sqr(sphere.Radius);
	}

	inline float planeClosestDistance(const Plane& plane, TVector3fArg pos)
	{
		return plane.DistanceToOrigin - dot(plane.Normal, pos); // something like this?
	}

	inline bool spherePlaneOvelap(const Sphere& sphere, const Plane& plane)
	{
		return sqr(planeClosestDistance(plane, sphere.Origin))<sqr(sphere.Radius);
	}

	inline Aabb3 stretchBox(const Aabb3& box, TVector3fArg delta)
	{
		return Aabb3(box.LeftBottomBack - delta, box.RightTopFront + delta);
	}

	inline Aabb3 stretchBox(const Aabb3& box, float delta)
	{
		return stretchBox(box, one3<Vector3f>()*delta);
	}

	bool testBoxPlanes(const Aabb3& box, std::function<bool(Plane)> testFunc);
	bool testBoxVerts(const Aabb3& box, std::function<bool(Vector3f)> testFunc);
	bool boxContains(const Aabb3& box, TVector3fArg pos);
	bool boxSphereOverlap(const Aabb3& box, const Sphere& sphere);

	bool unitTest();
}