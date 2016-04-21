#pragma once

#include <random>

#define _USE_MATH_DEFINES
#include <math.h>

#include <improbable/math/vector3f.h>
#include <improbable/math/coordinates.h>

namespace improbable {
	namespace math {

		typedef const Vector3f TVector3fRet;
		typedef const Vector3f& TVector3fArg;

		template<class T> T zero3()
		{
			return T(0,0,0);
		}
		template<class T> T one3()
		{
			return T(1, 1, 1);
		}
		template<class T> T unitX3()
		{
			return T(1, 0, 0);
		}
		template<class T> T unitY3()
		{
			return T(0, 1, 0);
		}
		template<class T> T unitZ3()
		{
			return T(0, 0, 1);
		}

		inline Vector3f operator*(TVector3fArg a, float b)
		{
			return Vector3f(a.X()*b, a.Y()*b, a.Z()*b);
		}
		inline Vector3f operator*(float a, TVector3fArg b)
		{
			return b*a;
		}

		inline Vector3f operator/(TVector3fArg a, float b)
		{
			auto oneOnB = 1.0f / b;
			return Vector3f(a.X()*oneOnB, a.Y()*oneOnB, a.Z()*oneOnB);
		}
		
		inline Vector3f toVector3f(const Coordinates& c)
		{
			return Vector3f(static_cast<float>(c.X()), static_cast<float>(c.Y()), static_cast<float>(c.Z()));
		}

		inline Coordinates operator+(const Coordinates& a, const Coordinates& b)
		{
			return Coordinates(a.X() + b.X(), a.Y() + b.Y(), a.Z() + b.Z());
		}

		inline Vector3f operator-(const Coordinates& a, const Coordinates& b)
		{
			return Vector3f(static_cast<float>(a.X() - b.X()), static_cast<float>(a.Y() - b.Y()), static_cast<float>(a.Z() - b.Z()));
		}

		inline Coordinates operator+(const Coordinates& a, TVector3fArg b)
		{
			return Coordinates(a.X() + b.X(), a.Y() + b.Y(), a.Z() + b.Z());
		}

		inline Vector3f operator+(TVector3fArg a, TVector3fArg b)
		{
			return Vector3f(a.X() + b.X(), a.Y() + b.Y(), a.Z() + b.Z());
		}

		inline Vector3f operator-(TVector3fArg a, TVector3fArg b)
		{
			return a + b*(-1.0f);
		}
		inline Vector3f operator*(TVector3fArg a, TVector3fArg b)
		{
			return Vector3f(a.X()*b.X(), a.Y()*b.Y(), a.Z()*b.Z());
		}

		inline Vector3f operator/(TVector3fArg a, TVector3fArg b)
		{
			return Vector3f(a.X() / b.X(), a.Y() / b.Y(), a.Z() / b.Z());
		}

		template <class T> T sqr(T v)
		{
			return v*v;
		}
		template <class T> T lerp(T a, T b, float i)
		{
			return a*(1.0f - i) + b*i;
		}

		inline float sqrMag(TVector3fArg v)
		{
			auto sqrs = v*v;
			return sqrs.X()+sqrs.Y()+sqrs.Z();
		}

		inline float mag(TVector3fArg v)
		{
			return sqrtf(sqrMag(v));
		}

		inline Vector3f normalize(TVector3fArg v)
		{
			auto sqrMag = v.X()*v.X() + v.Y()*v.Y() + v.Z()*v.Z();
			auto oneOnMag = 1.0f / sqrtf(sqrMag);
			return Vector3f(v.X()*oneOnMag, v.Y()*oneOnMag, v.Z()*oneOnMag);
		}

		inline float dot(TVector3fArg a, TVector3fArg b)
		{
			return a.X()*b.X() + a.Y()*b.Y() + a.Z()*b.Z();
		}

		inline Vector3f cross(TVector3fArg a, TVector3fArg b) {
			return Vector3f(a.Y()*b.Z() - a.Z()*b.Y(), -(a.X()*b.Z() - a.Z()*b.X()), a.X()*b.Y() - a.Y()*b.X());
		}

		inline bool isZero(float a, float ep)
		{
			return a < ep && a > -ep;
		}

		inline bool isZero(TVector3fArg a, float ep) {
			return isZero(a.X(), ep) && isZero(a.Y(), ep) && isZero(a.Z(), ep);
		}

		inline float randomFloat(unsigned int seed = 0)
		{
			if (seed == 0)
			{
				std::random_device rdSeed;
				seed = rdSeed();

			}
			std::mt19937 sd(seed);
			std::uniform_int_distribution<int> dist(0, 1000);
			auto res = dist(sd);
			return res / 1000.0f;
		}

		inline Vector3f randomPos()
		{
			return Vector3f(randomFloat(), randomFloat(), randomFloat()) - one3<Vector3f>() / 2;
		}

		inline int sumOfN(int n)
		{
			return (n*(n + 1)) / 2;
		}

		extern const float epsilon;
		extern const float ln2;
		extern const float pi;
		extern const float cosPi;

		inline float toRadians(float degrees)
		{
			return (degrees / 180.0f)*pi;
		}

		class Quat;
		typedef const Quat TQuatRet;
		typedef const Quat& TQuatArg;

		//---------------------------
		class Quat
		{
		public:
			Quat() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}
			
			static TQuatRet identity() { return Quat(); }
			static TQuatRet rotateFromTo(TVector3fArg from, TVector3fArg to);
			static TQuatRet axisAngle(TVector3fArg axis, float ang);
			
			TVector3fRet operator*(TVector3fArg v) const
			{
				return Vector3f(
					(1 - 2 * (sqr(Y) + sqr(Z)))*v.X() +	2 * (X*Y - Z*W)*v.Y() +				2 * (X*Z + Y*W)*v.Z(),
					2 * (X*Y + Z*W)*v.X() +				(1 - 2 * (sqr(X) + sqr(Z)))*v.Y() +	2 * (Y*Z - X*W)*v.Z(),
					2 * (X*Z - Y*W)*v.X() +				2 * (Y*Z + X*W)*v.Y() +				(1 - 2 * (sqr(X) + sqr(Y)))*v.Z()
					);
			}

			float angle() const 
			{
				return 2 * acosf(W);
			}
			bool buildAxis(Vector3f& ax, float* angOut=nullptr) const
			{
				float ang = angle();
				float sinHalfAngle = sinf(ang /2);
				if (fabs(sinHalfAngle) > epsilon)
				{
					ax = Vector3f(X, Y, Z) / sinHalfAngle;

					if (angOut != nullptr)
						*angOut = ang;

					return true;
				}
				// undefined
				return false;
			}

		private:
			Quat(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

			float X;
			float Y;
			float Z;
			float W;
		};

		bool testQuat(float ep);
	}
}