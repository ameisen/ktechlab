#include "quickvector.h"

#include <cmath>
#include <cassert>
#include <qdebug.h>
#include <iostream>
#include <algorithm>

using namespace std;

QuickVector::type QuickVector::at (int index) const {
	return values[getIndex(index)];
}

bool QuickVector::atPut (int index, const QuickVector::type val) {
	index = getIndex(index);
	if (index >= size_) return false;

	values[index] = val;
	isChanged = true;
	return true;
}

bool QuickVector::atAdd(int index, const QuickVector::type val) {
	index = getIndex(index);
	if (index >= size_) return false;

	values[index] += val;
	isChanged = true;
	return true;
}

QuickVector::QuickVector(int size) :
	size_(size)
{
	assert(size >= 0);
	values = std::make_unique<QuickVector::type[]>(size);
	memset(values.get(), 0, sizeof(QuickVector::type) * size);
}

QuickVector::QuickVector(QuickVector &&old) :
	values(std::move(old.values)),
	size_(old.size_),
	isChanged(old.isChanged)
{
	old.size_ = 0;
	old.isChanged = true;
}

QuickVector::QuickVector(const QuickVector &old) :
	QuickVector(&old)
{}

QuickVector::QuickVector(const QuickVector *old) :
	size_(old->size_),
	isChanged(old->isChanged)
{
	assert(size_ >= 0);
	values = std::make_unique<QuickVector::type[]>(size_);

	memcpy(values.get(), old->values.get(), sizeof(QuickVector::type) * size_);
}

void QuickVector::fillWithZeros() {
	memset(values.get(), 0, sizeof(QuickVector::type) * size_);
	isChanged = true;
}

QuickVector QuickVector::operator - (const QuickVector &y) const {
	const int minSize = std::min(size_, y.size_);

	QuickVector ret = QuickVector{minSize};

	for (int i : Times{minSize}) {
		ret.values[i] = values[i] - y.values[i];
	}

	return ret;
}

void QuickVector::dumpToAux() const {
	for (int i : Times{size_}) {
		cout << values[i] << ' ';
	}
	cout << endl;
}

bool QuickVector::swapElements(int indexA, int indexB) {
	indexA = getIndex(indexA);
	indexB = getIndex(indexB);
	if (indexA >= size_ || indexB >= size_) return false;
	if (indexA == indexB) return true;

	std::swap(values[indexA], values[indexB]);
	isChanged = true;

	return true;
}

QuickVector & QuickVector::operator = (QuickVector &&y) {
	values = std::move(y.values);
	size_ = y.size_;
	isChanged = true;

	y.size_ = 0;
	y.isChanged = true;

	return *this;
}

QuickVector & QuickVector::operator = (const QuickVector &y) {
	if (size_ != y.size_) {
		values = std::make_unique<QuickVector::type[]>(y.size_);
		size_ = y.size_;
	}

	memcpy(values.get(), y.values.get(), sizeof(QuickVector::type) * size_);

	isChanged = true;
	return *this;
}

QuickVector & QuickVector::operator *= (const QuickVector &y) {
	if (size_ == 0 || y.size_ == 0) {
		return *this;
	}

	for (int i : Times{std::min(size_, y.size_)}) {
		values[i] *= y.values[i];
	}
	isChanged = true;
	return *this;
}

QuickVector & QuickVector::operator += (const QuickVector &y) {
	if (size_ == 0 || y.size_ == 0) {
		return *this;
	}

	for (int i : Times{std::min(size_, y.size_)}) {
		values[i] += y.values[i];
	}
	isChanged = true;
	return *this;
}
