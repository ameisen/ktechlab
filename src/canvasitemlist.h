#pragma once

#include "pch.hpp"

#include <QList>
#include <QPointer>

#include "canvasitems.h"

#include <map>

using SortedCanvasItems = std::multimap<double, QPointer<KtlQCanvasItem>>;

class KtlQCanvasItemList : public QPtrList<KtlQCanvasItem> {
	public:
	KtlQCanvasItemList() = default;
	KtlQCanvasItemList(const KtlQCanvasItemList &) = default;
	KtlQCanvasItemList(KtlQCanvasItemList &&) = default;
	KtlQCanvasItemList(const QList<KtlQCanvasItem *> &list) {
		reserve(list.size());
		for (auto *item : list) {
			if (!item) continue;
			*this << item;
		}
	}

	void sort();
	//KtlQCanvasItemList operator + (const KtlQCanvasItemList &l) const { return ((QPtrList<KtlQCanvasItem> &)*this) + l; }

	bool CleanUp() {
		bool changed = false;
		for (auto it = begin(); it != end();) {
			if (!*it) {
				it = erase(it);
				changed = true;
			}
			else {
				++it;
			}
		}
		return changed;
	}

	operator QList<KtlQCanvasItem *> () const {
		QList<KtlQCanvasItem *> result;
		result.reserve(size());
		for (auto &item : *this) {
			if (!item) continue;
			result << item;
		}
		return result;
	}

	template <typename T>
	bool contains (const QPointer<T> &ptr) const {
		for (auto &item : *this) {
			if (!item) continue;
			using QObjectPtr = QObject *;
			if (QObjectPtr(item.data()) == QObjectPtr(ptr.data())) {
				return true;
			}
		}
		return false;
	}
};
