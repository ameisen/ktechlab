#pragma once

#include "pch.hpp"

#include <memory>

// TODO : remove me
#ifndef CUI
#	define CUI	const unsigned int
#endif

class QuickVector final {
public:
	using type = double;

private:
	int getIndex(const int index) const {
		if (index >= 0) {
			return index;
		}
		// If the index is negative, it is number of elements from the back
		return size_ + index;
	}

public:
	static constexpr const type EPSILON = type(0.000001);

	QuickVector() = default;
	QuickVector(int size);
	QuickVector(QuickVector &&old);
	QuickVector(const QuickVector &old);
	QuickVector(const QuickVector *old);

	type & operator [] (const int i) {
		isChanged = true;
		return values[getIndex(i)];
	}

	type operator [] (const int i) const {
		return values[getIndex(i)];
	}

	operator type * () {
		isChanged = true;
		return values.get();
	}

	operator const type * () const {
		return values.get();
	}

	type at (int index) const;
	bool atPut (int index, const type val);
	bool atAdd (int index, const type val);

	int size() const { return size_; }

	void fillWithZeros();
	bool swapElements(int indexA, int indexB);

	QuickVector & operator = (QuickVector &&y);
	QuickVector & operator = (const QuickVector &y);
	QuickVector & operator *= (const type y);
	QuickVector & operator *= (const QuickVector &y);
	QuickVector & operator += (const QuickVector &y);
	QuickVector operator - (const QuickVector &y) const;

	void dumpToAux() const;

private:
	std::unique_ptr<type[]> values = {};
	int size_ = 0;

public:
	// This can just be public due to its expected and accepted use patterns
	bool isChanged = false;
};
