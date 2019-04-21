#ifndef SY_COMMON_H
#define SY_COMMON_H

#include <cstdint>
#include <cmath>
#include <sstream>
#include <string>
#include <algorithm>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

#define PI 3.141592654f

namespace util {
	inline f32 lerp(f32 a, f32 b, f32 t) {
		return (1.0f - t) * a + b * t;
	}

	template <typename T>
	inline std::string to_string_prec(const T a_value, const int n = 6) {
		std::ostringstream out;
		out.precision(n);
		out << std::fixed << a_value;
		return out.str();
	}

	inline bool hits(i32 x, i32 y, i32 bx, i32 by, i32 bw, i32 bh) {
		return x >= bx &&
			x <= bx + bw &&
			y >= by &&
			y <= by + bh;
	}
	
	inline bool overlap(i32 a, i32 alen, i32 b, i32 blen) {
		i32 highestStartPoint = std::max(a, b);
		i32 lowestEndPoint = std::min(a + alen, b + blen);
		return highestStartPoint < lowestEndPoint;
	}

	inline bool hits(i32 x, i32 y, i32 w, i32 h, i32 bx, i32 by, i32 bw, i32 bh) {
		return overlap(x, w, bx, bw) && overlap(y, h, by, bh);
	}
}

#endif // SY_COMMON_H