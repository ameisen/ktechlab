#pragma once

#include <QPoint>
#include <cmath>

static constexpr inline int roundDown(int x, int roundness) {
	if (x < 0)
		return (x - roundness + 1) / roundness;
	else
		return (x / roundness);
}
static constexpr inline int roundDown(double x, int roundness) {
	return roundDown(int(x), roundness);
}

static inline QPoint roundDown(const QPoint &p, int roundness) {
	return QPoint(
		roundDown(p.x(), roundness),
		roundDown(p.y(), roundness)
	);
}

static constexpr inline int toCanvas(int pos) {
  return pos * 8 + 4;
}

static constexpr inline int fromCanvas(int pos) {
	return roundDown(pos - 4, 8);
}

static inline QPoint toCanvas(const QPoint *pos) {
	return QPoint(
		toCanvas(pos->x()),
		toCanvas(pos->y())
	);
}

static inline QPoint fromCanvas(const QPoint *pos) {
	return QPoint(
		fromCanvas(pos->x()),
		fromCanvas(pos->y())
	);
}

static inline QPoint toCanvas(const QPoint &pos) {
	return toCanvas(&pos);
}

static inline QPoint fromCanvas(const QPoint &pos) {
	return fromCanvas(&pos);
}

static constexpr inline int roundDouble(double x) {
	return int((x >= 0.0) ? (x + 0.5) : (x - 0.5));
}

static inline double qpoint_distance_sq(const QPoint &p1, const QPoint &p2) {
	const auto dx = p1.x() - p2.x();
	const auto dy = p1.y() - p2.y();

	return (dx * dx) + (dy * dy);
}

static inline double qpoint_distance(const QPoint &p1, const QPoint &p2) {
	return std::sqrt(qpoint_distance_sq(p1, p2));
}

static constexpr inline int snapToCanvas(int x) {
	return roundDown(x, 8) * 8 + 4;
}
static constexpr inline int snapToCanvas(double x) {
	return snapToCanvas(roundDouble(x));
}
