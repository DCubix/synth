#ifndef SY_COMMON_H
#define SY_COMMON_H

#include <cstdint>
#include <cmath>
#include <sstream>
#include <string>
#include <algorithm>
#include <array>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <optional>

#ifndef SYN_PERF_TESTS
#	include <filesystem>
namespace fs = std::filesystem;
#endif

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

#include "json.hpp"
using Json = nlohmann::json;
namespace util {
	inline f32 lerp(f32 a, f32 b, f32 t) {
		return (1.0f - t) * a + b * t;
	}

	inline bool overlap(i32 a, i32 alen, i32 b, i32 blen) {
		i32 highestStartPoint = std::max(a, b);
		i32 lowestEndPoint = std::min(a + alen, b + blen);
		return highestStartPoint < lowestEndPoint;
	}

	inline bool hits(i32 x, i32 y, i32 w, i32 h, i32 bx, i32 by, i32 bw, i32 bh) {
		return overlap(x, w, bx, bw) && overlap(y, h, by, bh);
	}

	inline f32 deCasteljau(f32 a, f32 b, f32 c, f32 d, f32 t) {
		return std::pow(1.0f - t, 3)* a +
			3.0f * std::pow(1.0f - t, 2) * t * b +
			3.0f * (1.0f - t) * std::pow(t, 2) * c +
			std::pow(t, 3) * d;
	}

	template <typename T, size_t N>
	class Stack {
	public:
		Stack() : m_top(-1) {}

		inline bool empty() { return m_top <= -1; }
		inline void push(T value) {
			m_data[++m_top] = value;
		}
		inline std::optional<T> pop() {
			if (empty()) return {};
			return m_data[m_top--];
		}

		inline void clear(T val) { m_top = -1; m_data.fill(val); }

	private:
		std::array<T, N> m_data;
		i32 m_top{ -1 };
	};

	template <typename T, size_t N>
	class RingBuffer {
	public:
		inline bool push(T value) {
			u32 next = m_head + 1;
			if (next >= N) next = 0;
			if (next == m_tail) return false;
			m_data[m_head] = value;
			m_head = next;
			return true;
		}

		inline std::optional<T> pop() {
			if (m_head == m_tail) return {};
			u32 next = m_tail + 1;
			if (next >= N) next = 0;

			T data = m_data[m_tail];
			m_tail = next;
			return data;
		}

		inline T* data() { return m_data.data(); }

	private:
		std::array<T, N> m_data;
		u32 m_head{ 0 }, m_tail{ 0 };
	};

#ifndef SYN_PERF_TESTS
	namespace pack {

		inline void saveFile(const std::string& path, const Json& json) {
			fs::path pt(path);
			pt.replace_extension(".sprog");
			
			std::ofstream fp(pt.string());
			if (fp.good()) {
				fp << std::setw(4) << json;
				fp.close();
			}
		}

		inline Json loadFile(const std::string& path) {
			Json json;
			std::ifstream fp(path);
			if (fp.good()) {
				fp >> json;
			}
			return json;
		}
	}
#endif

}

#endif // SY_COMMON_H
