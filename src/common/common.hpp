#pragma once

#include "common/platform.hpp"

#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cmath>
#include <functional>
#include <array>

#include <KLocalizedString>
#include <KDialog>
#include <KTextEdit>

#include <QPointer>
#include <QString>
#include <QSize>
#include <QPoint>
#include <QPointF>
#include <QVector2D>
#include <QVector3D>
#include <QList>
#include <QSet>
#include <QObject>
#include <QDebug>
#include <QColor>
#include <QMap>
#include <QTimer>

#include <ktlconfig.h>

#include "common/types.hpp"
using namespace KTechLab::Types;

// Once concepts are widely supported, things like this can go away.
#define _enable_if(expr) std::enable_if_t<(expr), int> = 0

namespace Type {
	// A type is a pointer type if it is, well, a pointer or derives from QPointer
	template <typename T> static constexpr const bool isPointer =
		std::is_pointer_v<T> ||
		std::is_base_of_v<QObject, std::decay_t<T>>;
}

template <typename T> using QPtrList = QList<QPointer<T>>;
template <typename T> using QPtrSet = QSet<QPointer<T>>;
template <typename T> using QPtrVector = QVector<QPointer<T>>;

template <typename T, typename U> using enable_if_ref = typename std::enable_if<
	std::is_same<
		typename std::decay<T>::type,
		typename std::decay<U>::type
	>::value,
	T
>::type;

template <typename T>
class _Point {
	protected:
	_Point() = default;

	static constexpr const bool integral = std::is_integral_v<T>;

	template <typename U>
	static constexpr T roundValue(U value) {
		if constexpr (std::is_integral_v<U> || !integral) {
			return value;
		}
		return T(std::round(value));
	}

	static constexpr bool equal(const T &l, const T &r) {
		return l == r;
	}

	static constexpr bool not_equal(const T &l, const T &r) {
		return l != r;
	}
};

template <typename T>
class Point2 : _Point<T> {
	using Super = _Point<T>;
	public:
	union {
		T x = {};
		T width;
	};
	union {
		T y = {};
		T height;
	};

	constexpr Point2 round() const {
		if constexpr (!Super::integral) {
			return {
				std::round(x),
				std::round(y)
			};
		}
		return *this;
	}

	constexpr Point2() = default;
	constexpr Point2(const Point2 &) = default;
	template <typename U>
	constexpr Point2(U x, U y, bool round = true) :
		x(round ? Super::roundValue(x) : x),
		y(round ? Super::roundValue(y) : y)
	{}
	Point2(const QSize &p, bool round = true) : Point2(p.width(), p.height(), round) {}
	Point2(const QPoint &p, bool round = true) : Point2(p.x(), p.y(), round) {}
	Point2(const QPointF &p, bool round = true) : Point2(p.x(), p.y(), round) {}
	Point2(const QVector2D &p, bool round = true) : Point2(p.x(), p.y(), round) {}

	operator QSize() const {
		return QSize(x, y);
	}

	operator QPoint() const {
		return QPoint(x, y);
	}

	operator QPointF() const {
		return QPointF(x, y);
	}

	operator QVector2D() const {
		return QVector2D(x, y);
	}

	bool operator == (const Point2 &other) const {
		return
			Super::equal(x, other.x) &&
			Super::equal(y, other.y);
	}

	bool operator != (const Point2 &other) const {
		return
			Super::not_equal(x, other.x) ||
			Super::not_equal(y, other.y);
	}
};

template <typename T>
class Point3 : _Point<T> {
	using Super = _Point<T>;
	public:
	union {
		T x = {};
		T r;
		T width;
	};
	union {
		T y = {};
		T g;
		T height;
	};
	union {
		T z = {};
		T b;
		T depth;
	};

	constexpr Point3 round() const {
		if constexpr (!Super::integral) {
			return {
				std::round(x),
				std::round(y),
				std::round(z)
			};
		}
		return *this;
	}

	constexpr Point3() = default;
	constexpr Point3(const Point3 &) = default;
	template <typename U>
	constexpr Point3(U x, U y, U z = {}, bool round = true) :
		x(round ? Super::roundValue(x) : x),
		y(round ? Super::roundValue(y) : y),
		z(round ? Super::roundValue(z) : z)
	{}
	Point3(const QSize &p, bool round = true) : Point3(p.width(), p.height(), {}, round) {}
	Point3(const QPoint &p, bool round = true) : Point3(p.x(), p.y(), {}, round) {}
	Point3(const QPointF &p, bool round = true) : Point3(p.x(), p.y(), {}, round) {}
	Point3(const QVector2D &p, bool round = true) : Point3(p.x(), p.y(), {}, round) {}
	Point3(const QVector3D &p, bool round = true) : Point3(p.x(), p.y(), p.z(), round) {}

	operator QSize() const {
		return QSize(x, y);
	}

	operator QPoint() const {
		return QPoint(x, y);
	}

	operator QPointF() const {
		return QPointF(x, y);
	}

	operator QVector2D() const {
		return QVector2D(x, y);
	}

	operator QVector3D() const {
		return QVector3D(x, y, z);
	}

	bool operator == (const Point3 &other) const {
		return
			Super::equal(x, other.x) &&
			Super::equal(y, other.y) &&
			Super::equal(z, other.z);
	}

	bool operator != (const Point3 &other) const {
		return
			Super::not_equal(x, other.x) ||
			Super::not_equal(y, other.y) ||
			Super::not_equal(z, other.z);
	}
};

template <typename T> using Size2 = Point2<T>;
template <typename T> using Size3 = Point3<T>;

template <typename T>
static inline T iround(float x) {
	return T(std::clamp(
		T((x >= 0.0f) ? (x + 0.5f) : (x - 0.5f)),
		std::numeric_limits<T>::lowest(),
		std::numeric_limits<T>::max()
	));
}

template <typename T>
static inline T iround(double x) {
	return T(std::clamp(
		T((x >= 0.0) ? (x + 0.5) : (x - 0.5)),
		std::numeric_limits<T>::lowest(),
		std::numeric_limits<T>::max()
	));
}

template <typename T>
static inline T iround(long double x) {
	return T(std::clamp(
		T((x >= 0.0L) ? (x + 0.5L) : (x - 0.5L)),
		std::numeric_limits<T>::lowest(),
		std::numeric_limits<T>::max()
	));
}

static inline int round_int(float x) { return iround<int>(x); }
static inline int round_int(double x) { return iround<int>(x); }
static inline int round_int(long double x) { return iround<int>(x); }

static inline float saturate(float x) { return std::clamp(x, 0.0f, 1.0f); }
static inline double saturate(double x) { return std::clamp(x, 0.0, 1.0); }
static inline long double saturate(long double x) { return std::clamp(x, 0.0L, 1.0L); }

static inline float lerp(float s, float x, float y) { return x * (1.0f - s) + y * s; }
static inline double lerp(double s, double x, double y) { return x * (1.0 - s) + y * s; }
static inline long double lerp(long double s, long double x, long double y) { return x * (1.0L - s) + y * s; }

static inline float clamped_lerp(float s, float x, float y) { return std::clamp(lerp(s, x, y), std::min(x, y), std::max(x, y)); }
static inline double clamped_lerp(double s, double x, double y) { return std::clamp(lerp(s, x, y), std::min(x, y), std::max(x, y)); }
static inline long double clamped_lerp(long double s, long double x, long double y) { return std::clamp(lerp(s, x, y), std::min(x, y), std::max(x, y)); }

static inline float rlerp(float v, float x, float y) { return (v - x) / (y - x); }
static inline double rlerp(double v, double x, double y) { return (v - x) / (y - x); }
static inline long double rlerp(long double v, long double x, long double y) { return (v - x) / (y - x); }

static inline float clamped_rlerp(float s, float x, float y) { return std::clamp(rlerp(s, x, y), 0.0f, 1.0f); }
static inline double clamped_rlerp(double s, double x, double y) { return std::clamp(rlerp(s, x, y), 0.0, 1.0); }
static inline long double clamped_rlerp(long double s, long double x, long double y) { return std::clamp(rlerp(s, x, y), 0.0L, 1.0L); }

__attribute__((always_inline))
static inline QString i18n (const QString &str) {
	return i18n(str.toLocal8Bit().data());
}

__attribute__((always_inline))
static inline QString i18n (QString &&str) {
	return i18n(str.toLocal8Bit().data());
}

__attribute__((always_inline))
static inline QString localize(const char *text) {
	if (text == nullptr || text[0] == '\0')
		return QString{};
	return i18n(text);
}

template <typename... Tt>
__attribute__((always_inline))
static inline QString localize(const char *text, Tt && ... args) {
	if (text == nullptr || text[0] == '\0')
		return QString{};
	return i18n(text, std::forward<Tt>(args)...);
}

namespace Ptr {
	template <typename T>
	static inline bool isNull(const QPointer<T> &ptr) {
		return ptr.isNull() || !ptr;
	}

	template <typename T>
	static inline constexpr bool isNull(T *ptr) {
		return !ptr;
	}

	// TODO : unsafe
	template <typename T>
	static inline bool isNull(const std::weak_ptr<T> &ptr) {
		return ptr.expired();
	}

	template <typename T>
	static inline bool isValid(const QPointer<T> &ptr) {
		return !isNull(ptr);
	}

	template <typename T>
	static inline constexpr bool isValid(T *ptr) {
		return !isNull(ptr);
	}

	// TODO : unsafe
	template <typename T>
	static inline bool isValid(const std::weak_ptr<T> &ptr) {
		return !isNull(ptr);
	}

	template <typename T>
	static inline T * get(const QPointer<T> &ptr) {
		// TODO : I don't think the branch is necessary.
		return isNull(ptr) ? nullptr : ptr.data();
	}

	template <typename T>
	static inline constexpr T * get(T *ptr) {
		return isNull(ptr) ? nullptr : ptr;
	}

	// TODO : unsafe
	template <typename T>
	static inline T * get(const std::weak_ptr<T> &ptr) {
		return isNull(ptr) ? nullptr : ptr.data();
	}

	template <typename... Tt>
	static inline bool anyNull(const Tt& ...ptrs) {
		bool predicate = false;
		std::initializer_list<bool>{ ( predicate |= isNull(ptrs))... };
		return predicate;
	}

	template <typename... Tt>
	static inline bool allNull(const Tt& ...ptrs) {
		bool predicate = true;
		std::initializer_list<bool>{ ( predicate &= isNull(ptrs))... };
		return predicate;
	}
}

namespace Container {
	template <typename T> using Predicate = std::function<bool(const T &)>;

	enum class Option : uint {
		None = 0u,
		SkipNull = 1u << 0
	};

	static inline constexpr Option operator | (Option l, Option r) {
		using enum_t = std::underlying_type_t<Option>;
		return Option(enum_t(l) | enum_t(r));
	}

	static inline constexpr bool hasOption(const Option options, const Option option) {
		using enum_t = std::underlying_type_t<Option>;
		return (enum_t(options) & enum_t(option)) == enum_t(option);
	}

	// TODO : make a move-reference predicate version
	template <typename T>
	static inline void removeIf(QList<T> &list, const Predicate<T> &predicate, Option options = Option::None) {
		for (auto i = list.begin(); i != list.end();) {
			const auto &value = *i;
			if constexpr (Type::isPointer<T>) {
				if (hasOption(options, Option::SkipNull) && Ptr::isNull(value)) {
					++i;
					continue;
				}
			}

			if (predicate(value)) {
				i = list.erase(i);
			}
			else {
				++i;
			}
		}
	}

	template <typename T, _enable_if(Type::isPointer<T>)>
	static inline void removeNull(QList<T> &list) {
		removeIf(list, [](const T &value){ return Ptr::isNull(value); });
	}
}

template <uintsz N1, uintsz N2>
static inline constexpr std::array<char, N1 + N2> GetCategoryID(const char (&category)[N1], const char (&id)[N2]) {
	std::array<char, N1 + N2> result = {};

	for (uintsz i = 0; i < (N1 - 1); ++i) {
		result[i] = category[i];
	}

	result[N1 - 1] = '/';

	for (uintsz i = 0; i < N2; ++i) {
		result[N1 + i] = id[i];
	}

	return result;
}

using voltage_t = real;
using current_t = real;
using resistance_t = real;
using conductance_t = real;
using capacitance_t = real;
using power_t = real;

template <typename T>
struct Range {
	const T start_;
	const T end_;

	constexpr Range (T start, T end) :
		start_(start), end_(end) {}

	struct iterator final {
		T value_;
		const T endValue_;

		T getNext () const {
			assert(value_ != endValue_);

			if (endValue_ >= value_) {
				return value_ + 1;
			}
			return value_ - 1;
		}

		T getPrevious () const {
			// TODO : Assert when returning to start value

			if (endValue_ >= value_) {
				return value_ - 1;
			}
			return value_ + 1;
		}

		constexpr iterator (T value, T endValue) : value_(value), endValue_(endValue) {}

		iterator & operator ++ () {
			value_ = getNext();
			return *this;
		}

		constexpr iterator operator ++ (int) const {
			return iterator{getNext(), endValue_};
		}

		iterator & operator -- () {
			value_ = getPrevious();
			return *this;
		}

		constexpr iterator operator -- (int) const {
			return iterator{getPrevious(), endValue_};
		}

		constexpr bool operator == (const iterator &other) const {
			return value_ == other.value_;
		}

		constexpr bool operator != (const iterator &other) const {
			return value_ != other.value_;
		}

		constexpr T & operator * () {
			return value_;
		}

		constexpr T operator * () const {
			return value_;
		}
	};

	constexpr iterator begin () const {
		return iterator{start_, end_};
	}

	constexpr iterator end () const {
		return iterator{end_, end_};
	}

	constexpr iterator rbegin () const {
		// TODO : fix for unsigned
		return iterator{end_ - 1, start_ - 1};
	}

	constexpr iterator rend () const {
		// TODO : fix for unsigned
		return iterator{start_ - 1, start_ - 1};
	}
};

template <typename T> using From = Range<T>;
template <typename T> using Until = Range<T>;

template <typename T>
struct Times : public Range<T> {
	constexpr Times(T times) : Range<T>(0, times) {}
};

// TODO : should be long maybe, since int * int max size > int
static inline int GetArea(const QRect &rect) {
	return rect.width() * rect.height();
}

static inline int GetArea(const QSize &size) {
	return size.width() * size.height();
}

class QPointRect final : public QRect {
public:
	QPointRect() : QRect(0, 0, 1, 1) {}
	QPointRect(const QPointRect &rect) = default;
	QPointRect(QPointRect &&rect) = default;
	QPointRect(int x, int y) : QRect(x, y, 1, 1) {}
	QPointRect(int edge) : QPointRect(edge, edge) {}
	QPointRect(const QPoint &point) : QPointRect(point.x(), point.y()) {}
};

template <typename T>
static inline uint qHash(const QPointer<T> &ptr, uint seed) {
	return qHash(ptr ? ptr.data() : nullptr, seed);
}

template <typename T>
static inline uint qHash(const QPointer<T> &ptr) {
	return qHash(ptr ? ptr.data() : nullptr);
}
