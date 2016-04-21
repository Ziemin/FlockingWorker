
#include "geometry.h"

//#pragma optimize("", off)

namespace geometry
{
	//*********************************************************************************
	bool testBoxPlanes(const Aabb3& box, std::function<bool(Plane)> testFunc)
	{
		//left
		if (!testFunc(Plane(unitX3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		//right
		if (!testFunc(Plane(unitX3<Vector3f>(), box.RightTopFront)))
			return false;
		//bottom
		if (!testFunc(Plane(unitY3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		//top
		if (!testFunc(Plane(unitY3<Vector3f>(), box.RightTopFront)))
			return false;
		//back
		if (!testFunc(Plane(unitZ3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		//front
		if (!testFunc(Plane(unitZ3<Vector3f>(), box.RightTopFront)))
			return false;

		return true;
	}
	//*********************************************************************************
	bool testBoxVerts(const Aabb3& box, std::function<bool(Vector3f)> testFunc)
	{
		if (testFunc(box.LeftBottomBack*Vector3f(1, 1, 1) + box.RightTopFront*Vector3f(0, 0, 0)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(0, 1, 1) + box.RightTopFront*Vector3f(1, 0, 0)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(1, 0, 1) + box.RightTopFront*Vector3f(0, 1, 0)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(1, 1, 0) + box.RightTopFront*Vector3f(0, 0, 1)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(0, 0, 0) + box.RightTopFront*Vector3f(1, 1, 1)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(1, 0, 0) + box.RightTopFront*Vector3f(0, 1, 1)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(0, 1, 0) + box.RightTopFront*Vector3f(1, 0, 1)))
			return true;
		if (testFunc(box.LeftBottomBack*Vector3f(0, 0, 1) + box.RightTopFront*Vector3f(1, 1, 0)))
			return true;

		return false;
	}

	//*********************************************************************************
	bool boxSphereOverlap(const Aabb3& box, const Sphere& sphere)
	{
		// early out if we have no chance of overlap
		if (!boxContains(stretchBox(box, one3<Vector3f>()*sphere.Radius), sphere.Origin))
			return false;

		if (boxContains(stretchBox(box, unitX3<Vector3f>()*sphere.Radius), sphere.Origin))
			return true;
		if (boxContains(stretchBox(box, unitY3<Vector3f>()*sphere.Radius), sphere.Origin))
			return true;
		if (boxContains(stretchBox(box, unitZ3<Vector3f>()*sphere.Radius), sphere.Origin))
			return true;

		// now just check the corners!
		if (testBoxVerts(box, [sphere](TVector3fArg vert) {
			return sqrMag(sphere.Origin - vert)<sqr(sphere.Radius);
		})) {
			return true;
		}

		return false;
	}

	bool boxContains(const Plane* planes, TVector3fArg pos)
	{
		auto correctSide = [pos](const Plane& plane)
		{
			return dot(plane.Normal, pos) < plane.DistanceToOrigin;
		};
		for (int cplane = 0; cplane < 6; ++cplane)
		{
			auto& plane = planes[cplane];
			if (!correctSide(plane))
			{
				return false;
			}
		}

		return true;
	}

	//*********************************************************************************
	bool boxContains(const Aabb3& box, TVector3fArg pos)
	{
		auto correctSide = [pos](const Plane& plane)
		{
			return dot(plane.Normal, pos) < plane.DistanceToOrigin;
		};
		
		//return testBoxPlanes(box, correctSide);

		if (!correctSide(Plane(unitX3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		if (!correctSide(Plane(unitX3<Vector3f>(), box.RightTopFront)))
			return false;
		if (!correctSide(Plane(unitY3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		if (!correctSide(Plane(unitY3<Vector3f>(), box.RightTopFront)))
			return false;
		if (!correctSide(Plane(unitZ3<Vector3f>()*-1.0f, box.LeftBottomBack)))
			return false;
		if (!correctSide(Plane(unitZ3<Vector3f>(), box.RightTopFront)))
			return false;

		return true;
	}

	//*********************************************************************************
	CubeSphereIntersection::CubeSphereIntersection(const Aabb3& box, float radius)
	{
		Plane* planeBuffer = Planes;

		auto writePlanes = [planeBuffer](int itarget, const Aabb3& box)
		{
			auto& lbb = box.LeftBottomBack;
			auto& rtf = box.RightTopFront;

			planeBuffer[itarget * 6 + 0] = Plane(unitX3<Vector3f>()*-1.0f, lbb);
			planeBuffer[itarget * 6 + 1] = Plane(unitX3<Vector3f>(), rtf);
			planeBuffer[itarget * 6 + 2] = Plane(unitY3<Vector3f>()*-1.0f, lbb);
			planeBuffer[itarget * 6 + 3] = Plane(unitY3<Vector3f>(), rtf);
			planeBuffer[itarget * 6 + 4] = Plane(unitZ3<Vector3f>()*-1.0f, lbb);
			planeBuffer[itarget * 6 + 5] = Plane(unitZ3<Vector3f>(), rtf);
		};

		writePlanes(SuperBox, stretchBox(box, one3<Vector3f>()*radius));
		writePlanes(StretchX, stretchBox(box, unitX3<Vector3f>()*radius));
		writePlanes(StretchY, stretchBox(box, unitY3<Vector3f>()*radius));
		writePlanes(StretchZ, stretchBox(box, unitZ3<Vector3f>()*radius));

		auto& lbb = box.LeftBottomBack;
		auto& rtf = box.RightTopFront;

		Vertices[0] = Sphere(lbb*Vector3f(1, 1, 1) + rtf*Vector3f(0, 0, 0), radius);
		Vertices[1] = Sphere(lbb*Vector3f(0, 1, 1) + rtf*Vector3f(1, 0, 0), radius);
		Vertices[2] = Sphere(lbb*Vector3f(1, 0, 1) + rtf*Vector3f(0, 1, 0), radius);
		Vertices[3] = Sphere(lbb*Vector3f(1, 1, 0) + rtf*Vector3f(0, 0, 1), radius);
		Vertices[4] = Sphere(lbb*Vector3f(0, 0, 0) + rtf*Vector3f(1, 1, 1), radius);
		Vertices[5] = Sphere(lbb*Vector3f(1, 0, 0) + rtf*Vector3f(0, 1, 1), radius);
		Vertices[6] = Sphere(lbb*Vector3f(0, 1, 0) + rtf*Vector3f(1, 0, 1), radius);
		Vertices[7] = Sphere(lbb*Vector3f(0, 0, 1) + rtf*Vector3f(1, 1, 0), radius);
	}

	//*********************************************************************************
	bool CubeSphereIntersection::IntersectionAt(TVector3fArg spherePos) const
	{
		// early out if we have no chance of overlap
		if (!boxContains(Planes+SuperBox * 6, spherePos))
			return false;

		if (boxContains(Planes + StretchX * 6, spherePos))
			return true;
		if (boxContains(Planes + StretchY * 6, spherePos))
			return true;
		if (boxContains(Planes + StretchZ * 6, spherePos))
			return true;

		// now just check the corners!
		for (int cvert = 0; cvert < 8; ++cvert)
		{
			if (sphereContains(Vertices[cvert], spherePos))
			{
				return true;
			}
		}
		return false;
	}

	//*********************************************************************************
	bool unitTest()
	{
		Vector3f boxLBB = Vector3f(50, 50, 50);
		Vector3f boxRTF = Vector3f(60, 60, 60);
		Aabb3 boxTest(boxLBB, boxRTF);

		printf("volume\n");
		{
			Sphere sphereOut = { Vector3f(0, 55, 55), 32 };
			Sphere sphereIn = { Vector3f(20, 55, 55), 32 };
			if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
			if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
		}{
			Sphere sphereOut = { Vector3f(55, 0, 55), 32 };
			Sphere sphereIn = { Vector3f(55, 20, 55), 32 };
			if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
			if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
		} {
			Sphere sphereOut = { Vector3f(55, 55, 0), 32 };
			Sphere sphereIn = { Vector3f(55, 55, 20), 32 };
			if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
			if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
		}
		// corner cases
		printf("corners\n");
		{
			Vector3f v0 = boxLBB*Vector3f(1, 1, 1) + boxRTF*Vector3f(0, 0, 0);
			Vector3f v1 = boxLBB*Vector3f(0, 1, 1) + boxRTF*Vector3f(1, 0, 0);
			Vector3f v2 = boxLBB*Vector3f(1, 0, 1) + boxRTF*Vector3f(0, 1, 0);
			Vector3f v3 = boxLBB*Vector3f(1, 1, 0) + boxRTF*Vector3f(0, 0, 1);

			Vector3f v4 = boxLBB*Vector3f(0, 0, 0) + boxRTF*Vector3f(1, 1, 1);
			Vector3f v5 = boxLBB*Vector3f(1, 0, 0) + boxRTF*Vector3f(0, 1, 1);
			Vector3f v6 = boxLBB*Vector3f(0, 1, 0) + boxRTF*Vector3f(1, 0, 1);
			Vector3f v7 = boxLBB*Vector3f(0, 0, 1) + boxRTF*Vector3f(1, 1, 0);

			{
				Sphere sphereOut = { v0 - one3<Vector3f>() * 20, 32 };
				Sphere sphereIn = { v0 - one3<Vector3f>() * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v1 - Vector3f(-1,1,1) * 20, 32 };
				Sphere sphereIn = { v1 - Vector3f(-1,1,1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v2 - Vector3f(1,-1,1) * 20, 32 };
				Sphere sphereIn = { v2 - Vector3f(1,-1,1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v3 - Vector3f(1,1,-1) * 20, 32 };
				Sphere sphereIn = { v3 - Vector3f(1,1,-1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v4 + one3<Vector3f>() * 20, 32 };
				Sphere sphereIn = { v4 + one3<Vector3f>() * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v5 + Vector3f(-1,1,1) * 20, 32 };
				Sphere sphereIn = { v5 + Vector3f(-1,1,1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v6 + Vector3f(1,-1,1) * 20, 32 };
				Sphere sphereIn = { v6 + Vector3f(1,-1,1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			} {
				Sphere sphereOut = { v7 + Vector3f(1,1,-1) * 20, 32 };
				Sphere sphereIn = { v7 + Vector3f(1,1,-1) * 15, 32 };
				if (!boxSphereOverlap(boxTest, sphereOut)) printf("success\n"); else printf("failure\n");
				if (boxSphereOverlap(boxTest, sphereIn)) printf("success\n"); else printf("failure\n");
			}
		}
		return true;
	}
}