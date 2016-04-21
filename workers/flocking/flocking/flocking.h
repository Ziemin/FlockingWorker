#pragma once

#include "Maths.h"
#include "demoteam/transform.h"

using namespace improbable::math;

namespace demoteam
{
#ifdef _WIN32
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif //_WIN32

	FORCEINLINE bool ShouldConsiderEntity(const TransformData& me, const TransformData& them, float range)
	{
		Vector3f lineTo = them.position() - me.position();

		// test range
		float distSqr = sqrMag(lineTo);
		if (distSqr > sqr(range) || distSqr<epsilon)
		{
			return false;
		}
		// in front of me?
		if (dot(lineTo, me.forward())<0.0f)
		{
			return false;
		}
		return true;
	}
}