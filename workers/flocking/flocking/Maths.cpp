
#include "Maths.h"

//#pragma optimize("", off)

namespace improbable
{
	namespace math
	{
		const float epsilon = 0.001f;
		const float ln2 = 0.6931f;
		const float pi = 3.14159f;
		const float cosPi = 0.0f;
		
		bool equals(float a, float b, float ep)
		{
			return isZero(a - b, ep);
		}

		bool vectorEquals(TVector3fArg a, TVector3fArg b, float ep)
		{
			auto diff = a - b;
			return isZero(dot(diff, unitX3<Vector3f>()), ep) &&
				isZero(dot(diff, unitY3<Vector3f>()), ep) &&
				isZero(dot(diff, unitZ3<Vector3f>()), ep);
		}
		
		TQuatRet Quat::axisAngle(TVector3fArg axis, float ang)
		{
			float sinHalfAngle = sinf(ang / 2);
			float cosHalfAngle = cosf(ang / 2);
			return Quat(axis.X()*sinHalfAngle, axis.Y()*sinHalfAngle, axis.Z()*sinHalfAngle, cosHalfAngle);
		}

		TQuatRet Quat::rotateFromTo(TVector3fArg unitFrom, TVector3fArg unitTo)
		{
			auto perp = cross(unitFrom, unitTo);

			if (isZero(perp, epsilon))
			{
				return Quat::identity();
			}

			auto unitPerp = normalize(perp);
			auto cosAng = dot(unitFrom, unitTo);
			auto ang = acosf(std::min(std::max(cosAng, -1.0f), 1.0f));
			return Quat::axisAngle(unitPerp, ang);

			//auto cosHalfAngle = sqrtf(std::max((cosAng + 1) / 2, 0.0f));
			//auto sinHalfAngle = sqrtf(std::max((1 - cosAng) / 2, 0.0f)); // plus minus oof
			//auto sinAng = std::max(1.0f - sqr(cosAng), 0.0f);
			//return Quat(unitPerp.X()*sinHalfAngle, unitPerp.Y()*sinHalfAngle, unitPerp.Z()*sinHalfAngle, cosHalfAngle);
		}
		
		bool testVector3fRotation(Vector3f from, Vector3f to)
		{
			bool res = true;

			auto rot = Quat::rotateFromTo(from, to);
			auto rotTest = rot*from;
			if (!vectorEquals(rotTest, to, epsilon))
			{
				// this is bogus!
				res = false;
			}

			Vector3f ax = zero3<Vector3f>();
			float ang;
			if (res && rot.buildAxis(ax, &ang))
			{
				auto rot2 = Quat::axisAngle(ax, ang);

				auto rotTest2 = rot2*from;
				if (!vectorEquals(rotTest2, to, epsilon))
				{
					// this is bogus!
					res = false;
				}
			}

			printf("testing (%.3f,%.3f,%.3f) against (%.3f,%.3f,%.3f); result: %d\n", from.X(), from.Y(), from.Z(), to.X(), to.Y(), to.Z(), res);

			return res;
		}

		Vector3f pointOnUnitSphere(float tt)
		{
			const float f = 100.0f;
			float t = 1 - 2 * tt;
			float r = 1 - sqr(t);
			float cosTerm = cosf(t*pi / 2);
			return normalize(Vector3f(r*sinf(f*t)*cosTerm, r*cosf(f*t)*cosTerm, 1.05f*tanhf(1.86f*t))); // todo: fix the maths so normalization is unnecessary
		}

		Vector3f randomUnitVector()
		{
			return pointOnUnitSphere(randomFloat());
		}

		bool testQuat(float ep)
		{
			// quat test
			auto rot = Quat::axisAngle(unitY3<Vector3f>(), pi / 4);
			Vector3f v = unitZ3<Vector3f>();
			for (int c0 = 0; c0 < 8; ++c0)
				v = rot*v;

			bool res = true;

			res = res && testVector3fRotation(unitX3<Vector3f>(), unitY3<Vector3f>());
			res = res && testVector3fRotation(unitY3<Vector3f>(), unitZ3<Vector3f>());
			res = res && testVector3fRotation(unitZ3<Vector3f>(), unitX3<Vector3f>());
			res = res && testVector3fRotation(unitY3<Vector3f>(), unitX3<Vector3f>());
			res = res && testVector3fRotation(unitZ3<Vector3f>(), unitY3<Vector3f>());
			res = res && testVector3fRotation(unitX3<Vector3f>(), unitZ3<Vector3f>());

			for (int c0 = 0; c0 < 100 && res; ++c0)
			{
				auto from = randomUnitVector();
				auto to = randomUnitVector();
				res = res && testVector3fRotation(from, to);
			}

			return res;
		}
	}
}