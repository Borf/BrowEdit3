#include <glm/glm.hpp>

namespace math
{


	class HermiteCurve
	{
	public:
		template<class T>
		static T getPoint(const T& p0, const T& m0, const T& p1, const T& m1, float t)
		{
			return (2 * t * t * t - 3 * t * t + 1) * p0 + (t * t * t - 2 * t * t + t) * m0 + (-2 * t * t * t + 3 * t * t) * p1 + (t * t * t - t * t) * m1;
		}

		template<class T>
		static T getPointAtDistance(const T& p0, const T& m0, const T& p1, const T& m1, float d)
		{
			float distUpToNow = 0;
			float stepUpToNow = 0;
			float step = 1;
			while (glm::abs(distUpToNow - d) > 0.1)
			{
				float len = getLength(p0, m0, p1, m1, stepUpToNow, stepUpToNow+step);
				if (distUpToNow + len < d)
				{
					distUpToNow += len;
					stepUpToNow += step;
					step = 1 - step;
				}

				step *= 0.5f;
			}

			return getPoint(p0, m0, p1, m1, stepUpToNow);
		}

		template<class T>
		static float getLength(const T& p0, const T& m0, const T& p1, const T& m1, float begin = 0.0f, float end = 1.0f)
		{
			float length = 0;
			T lastPoint = getPoint(p0, m0, p1, m1, begin);
			for (float f = begin; f < end; f += 0.01f)
			{
				T point = getPoint(p0, m0, p1, m1, f);
				length += glm::distance(point, lastPoint);
				lastPoint = point;
			}
			length += glm::distance(lastPoint, getPoint(p0, m0, p1, m1, end));
			return length;
		}

	};

}