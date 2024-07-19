#ifndef VECTOR2_H_
#define VECTOR2_H_
#include <cmath>
#include <iostream>
#include "Utils.h"
#include <type_traits>
#include <map>
#include <vector>

enum class ORIENTATION {RIGHT = 0, LEFT = 1, UNSPECIFIED = 2};
inline const std::map<ORIENTATION, const char *> OrientationNames {
	{ORIENTATION::RIGHT, "RIGHT"},
	{ORIENTATION::LEFT, "LEFT"},
	{ORIENTATION::UNSPECIFIED, "UNSPECIFIED"},
};

template <typename T>
struct Vector2
{
	T x, y;
	constexpr inline Vector2(T nx = 0, T ny = 0)
	{
		static_assert(std::is_arithmetic_v<T>, "Type T must be an arithmetic type");

		x = nx;
		y = ny;
	}

	template <typename TR>
	constexpr inline Vector2(const Vector2<TR> &rhs)
	{
		x = rhs.x;
		y = rhs.y;
	}

	template <typename TR>
	constexpr inline Vector2<T> &operator=(const Vector2<TR> &rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	template <typename TR>
	constexpr inline bool operator==(const Vector2<TR> &rhs) const
	{
		return (x == rhs.x && y == rhs.y);
	}

	constexpr inline bool operator==(const ORIENTATION &rhs_) const
	{
		return (x > 0 && rhs_ == ORIENTATION::RIGHT) || (x < 0 && rhs_ == ORIENTATION::LEFT) || (x == 0 && rhs_ == ORIENTATION::UNSPECIFIED);
	}

	template<ORIENTATION DEFAULT = ORIENTATION::UNSPECIFIED>
	constexpr inline ORIENTATION getOrientation() const
	{
		if (x > 0)
			return ORIENTATION::RIGHT;
		else if (x < 0)
			return ORIENTATION::LEFT;
		else
			return DEFAULT;
	}

	template<typename TR>
	constexpr inline auto operator+(const Vector2<TR>& rhs) const -> Vector2<decltype(x+rhs.x)>
	{
		return { x + rhs.x, y + rhs.y };
	}

	template<typename TR>
	constexpr inline auto operator-(const Vector2<TR>& rhs) const -> Vector2<decltype(x-rhs.x)>
	{
		return { x - rhs.x, y - rhs.y };
	}

	constexpr inline Vector2<T> operator-() const
	{
		return { -x, -y };
	}

	template<typename TR>
	constexpr inline auto operator*(const TR& num) const -> Vector2<decltype(x*num)>
	{
		return { x * num, y * num };
	}

	template<typename TR>
	constexpr inline auto operator/(const TR& num) const -> Vector2<decltype(x/num)>
	{
		return { x / num, y / num };
	}

    template<typename TR>
	constexpr inline Vector2<T> &operator+=(const Vector2<TR>& rhs)
	{
		x += rhs.x;
		y += rhs.y;
        return *this;
	}

    template<typename TR>
	constexpr inline Vector2<T> operator-=(const Vector2<TR>& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
        return *this;
	}

    template<typename TR>
	constexpr inline Vector2<T> operator*=(const TR& num)
	{
		x *= num;
		y *= num;
        return *this;
	}

    template<typename TR>
	constexpr inline Vector2<T> operator/=(const TR& num)
	{
		x /= num;
		y /= num;
        return *this;
	}

	constexpr inline auto getLen() const
	{
		return sqrt(x * x + y * y);
	}

	constexpr inline auto getSqLen() const
	{
		return x * x + y * y;
	}

	constexpr inline Vector2<decltype(x / sqrt(x))> normalised() const
	{
		float l = sqrt(x * x + y * y);
		if (l == 0.0f)
			return *this;

		return {x / l, y / l};
	}

    template<typename TR>
	constexpr inline auto mulComponents(const Vector2<TR>& rhs) const -> Vector2<decltype(x * rhs.x)>
	{
		return { x * rhs.x, y * rhs.y };
	}

	template<bool ON_ZEROES = true, typename T2>
	constexpr inline bool areAlignedOnX(const Vector2<T2> &rhs_) const
	{
		return utils::sameSign<ON_ZEROES, T, T2>(x, rhs_.x);
	}

	template<bool ON_ZEROES = true, typename T2>
	constexpr inline bool areAlignedOnY(const Vector2<T2> &rhs_) const
	{
		return utils::sameSign<ON_ZEROES, T, T2>(y, rhs_.y);
	}
};

template <typename T>
std::ostream& operator<< (std::ostream& out, const Vector2<T>& vec)
{
    out << vec.x << " " << vec.y;
    return out;
}

//Only rectangle hitbox
struct Collider
{
	float x, y, w, h;
	constexpr inline Collider(float nx = 0, float ny = 0, float nw = 0, float nh = 0)
	{
		x = nx;
		y = ny;
		w = nw;
		h = nh;
	}
	template<typename T1, typename T2>
	constexpr inline Collider(const Vector2<T1> &pos_, const Vector2<T2> &size_)
	{
		x = pos_.x;
		y = pos_.y;
		w = size_.x;
		h = size_.y;
	}
	constexpr inline Collider operator+(const Vector2<float>& rhs) const
	{
		return { x + rhs.x, y + rhs.y, w, h };
	}

	constexpr inline int checkOverlapWith_x(const Collider& hbox) const
	{
		if (x <= hbox.x && x + w > hbox.x)
			return 1;
		if (x + w >= hbox.x + hbox.w && x < hbox.x + hbox.w)
			return 2;
		if (x >= hbox.x && x + w <= hbox.x + hbox.w)
			return 4;
		else
			return 0;
	}
	constexpr inline int checkOverlapWith_y(const Collider& hbox) const
	{
		if (y <= hbox.y && y + h > hbox.y)
			return 1;
		if (y + h >= hbox.y + hbox.h && y < hbox.y + hbox.h)
			return 2;
		if (y >= hbox.y && y + h <= hbox.y + hbox.h)
			return 4;
		else
			return 0;
	}

	constexpr inline int checkCollisionWith_x(const Collider& hbox) const
	{
		if (x <= hbox.x && x + w >= hbox.x)
			return 1;
		if (x + w >= hbox.x + hbox.w && x <= hbox.x + hbox.w)
			return 2;
		if (x >= hbox.x && x + w <= hbox.x + hbox.w)
			return 4;
		else
			return 0;
	}
	constexpr inline int checkCollisionWith_y(const Collider& hbox) const
	{
		if (y <= hbox.y && y + h >= hbox.y)
			return 1;
		if (y + h >= hbox.y + hbox.h && y <= hbox.y + hbox.h)
			return 2;
		if (y >= hbox.y && y + h <= hbox.y + hbox.h)
			return 4;
		else
			return 0;
	}

	template<bool H_OVERLAP_ONLY, bool V_OVERLAP_ONLY>
	constexpr inline bool checkCollisionWith(const Collider& hbox) const
	{
		return (H_OVERLAP_ONLY ? checkOverlapWith_x(hbox) : checkCollisionWith_x(hbox)) &&
			   (V_OVERLAP_ONLY ? checkOverlapWith_y(hbox) : checkCollisionWith_y(hbox));
	}

	//-1 if beyond left bound
	// 1 if beyond right bound
	// 0 if within bounds
	constexpr inline int isWithinHorizontalBounds(float leftBound_, float rightBound_) const
	{
		if (x < leftBound_)
			return -1;
		
		if (x + w > rightBound_)
			return 1;

		return 0;
	}
	
	constexpr inline float rangeToLeftBound(float leftBound_) const
	{
		return x - leftBound_;
	}

	constexpr inline float rangeToRightBound(float rightBound_) const
	{
		return rightBound_ - (x + w);
	}

	constexpr inline bool doesExceedOrTouchBoundaries(float leftBound_, float rightBound_) const
	{
		if (x <= leftBound_)
			return true;
		
		if (x + w >= rightBound_)
			return true;

		return false;
	}


	constexpr inline Vector2<float> getPos() const
	{
		return {x, y};
	}
	constexpr inline Vector2<float> getSize() const
	{
		return {w, h};
	}
	constexpr inline Vector2<float> getCenter() const
	{
		return {x + w / 2.0f, y + h / 2.0f};
	}

	constexpr inline float getOwnOverlapPortion(const Collider &rhs_) const
	{
		Vector2<float> tl {
			std::max(x, rhs_.x),
			std::max(y, rhs_.y),
		};

		Vector2<float> br {
			std::min(x + w, rhs_.x + rhs_.w),
			std::min(y + h, rhs_.y + rhs_.h),
		};

		auto size = br - tl;
		if (size.x < 0 || size.y < 0)
			return 0;

		return (size.x * size.y) / (w * h);
	}
	
};

inline std::ostream& operator<< (std::ostream& out_, const Collider& cld_)
{
    out_ << "{ " << cld_.x << ", " << cld_.y << ", " << cld_.w << ", " << cld_.h << " }";
    return out_;
}

namespace utils
{
	template <typename T>
	inline Vector2<T> clamp(const Vector2<T>& val, const Vector2<T> &min, const Vector2<T> &max)
	{
		return {clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y)};
	}

	template <typename T>
	inline Vector2<T> limitVectorLength(const Vector2<T>& val, const T &limit)
	{
		auto curlen = val.getLen();
		if (curlen > limit)
		{
			return val / (curlen / limit);
		}

		return val;
	}

	template <typename T, typename aT>
	inline Vector2<T> lerp(const Vector2<T> &min, const Vector2<T> &max, const aT &alpha)
	{
		return {min + (max - min) * alpha};
	}

	inline std::vector<Vector2<int>> getAreaEdgePoints(int w_, int h_, const Vector2<int> &origin_, int len_)
    {
        int minx = origin_.x - len_;
        int maxx = origin_.x + len_;

        int miny = origin_.y - len_;
        int maxy = origin_.y + len_;

        std::vector<Vector2<int>> points;

        for (int x = minx; x <= maxx; ++x)
        {
            for (int y : {miny, maxy})
            {
                if (x >= 0 && x < w_ && y >= 0 && y < h_)
                    points.push_back({x, y});
            }
        }

        for (int y = miny + 1; y < maxy; ++y)
        {
            for (int x : {minx, maxx})
            {
                if (x >= 0 && x < w_ && y >= 0 && y < h_)
                    points.push_back({x, y});
            }
        }

        return points;
    }

	template <typename T>
	inline std::string toString(const Vector2<T> &v_)
	{
		std::stringstream s;
		s << "{" << v_.x << ", " << v_.y << "}";
		return s.str();
	}
}

#endif