#include "quickmatrix.h"

#include <cmath>
#include <cassert>
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

bool QuickMatrix::isSquare() const {
	return rows == columns;
}

QuickMatrix::type ** QuickMatrix::allocate() const {
	if (rows <= 0 || columns <= 0) {
		return nullptr;
	}

	auto **newValues = new type*[rows];
	for (int i : Times{rows}) {
		newValues[i] = new type[columns];
	}
	return newValues;
}

void QuickMatrix::release() {
	if (!values) return;
	for (int i : Times{rows}) {
		delete [] values[i];
	}
	delete [] values;
	values = nullptr;
}

QuickMatrix QuickMatrix::transposeSquare() const {
	QuickMatrix newmat = QuickMatrix{columns};

	for (int i : Times{columns}) {
		for (int j : Times{rows}) {
			newmat.values[i][j] = 0;
			for (int k : Times{rows}) {
				newmat.values[i][j] += values[k][i] * values[k][j];
			}
		}
	}

	return newmat;
}

QuickVector QuickMatrix::transposeMult(const QuickVector &operandvec) const {
	if (operandvec.size() != rows)
		return QuickVector{};

	QuickVector ret = QuickVector{columns};

	for (int i : Times{columns}) {
		type sum = 0;
		for (int j : Times{rows}) {
			sum += values[j][i] * operandvec[j];
		}
		ret[i] = sum;
	}

	return ret;
}

QuickMatrix::QuickMatrix(int _rows, int _columns) :
	rows(_rows),
	columns(_columns)
{
	values = allocate();
	fillWithZero();
}

QuickMatrix::QuickMatrix(int rows) :
	QuickMatrix(rows, rows)
{}

QuickMatrix::QuickMatrix(QuickMatrix &&old) :
	values(old.values),
	rows(old.rows),
	columns(old.columns)
{
	old.values = nullptr;
	old.rows = 0;
	old.columns = 0;
}

QuickMatrix::QuickMatrix(const QuickMatrix &old) :
	QuickMatrix(&old)
{}

QuickMatrix::QuickMatrix(const QuickMatrix *old) :
	rows(old->rows),
	columns(old->columns)
{
	values = allocate();

	for (int j : Times{rows}) {
		memcpy(values[j], old->values[j], columns * sizeof(type)); // fastest method. =)
	}
}

QuickMatrix::~QuickMatrix() {
	release();
}

QuickMatrix::type QuickMatrix::at(int row, int column) const {
	if (!values || row >= rows || column >= columns) return NAN;

	return values[row][column];
}

QuickMatrix::type QuickMatrix::multstep(int row, int pos, int col) const {
	return at(row, pos) * at(pos, col);
}

QuickMatrix::type QuickMatrix::multRowCol(int row, int col, int lim) const {
	type sum = 0;
	for (int i : Times{lim}) {
		sum += at(row, i) * at(i, col);
	}
	return sum;
}

bool QuickMatrix::atPut(int row, int column, const type val) {
	if (!values || row >= rows || column >= columns) return false;

	values[row][column] = val;
	return true;
}

bool QuickMatrix::atAdd(int row, int column, const type val) {
	if (!values || row >= rows || column >= columns) return false;

	values[row][column] += val;
	return true;
}

bool QuickMatrix::swapRows(int rowA, int rowB) {
	if (!values || rowA >= rows || rowB >= rows) return false;

	std::swap(values[rowA], values[rowB]);
	return true;
}

bool QuickMatrix::scaleRow(int row, const type scalor) {
	if (!values || row >= rows) return false;

	type *arow = values[row];
	for (int j : Times{columns}) {
		arow[j] *= scalor;
	}
	return true;
}

QuickMatrix::type QuickMatrix::rowsum(int row) const {
	if (!values || row >= rows) return false;

	const type *arow = values[row];

	type sum = 0.0;
	for (int j : Times{columns}) {
		sum += arow[j];
	}

	return sum;
}

double QuickMatrix::absrowsum(int row) const {
	if (!values || row >= rows) return false;

	const type *arow = values[row];

	type sum = 0.0;

	for (int j : Times{columns}) {
		sum += std::abs(arow[j]);
	}

	return sum;
}

// behaves oddly but doesn't crash if m_a == m_b
bool QuickMatrix::scaleAndAdd(int rowA, int rowB, const type scalor) {
	if (!values || rowA >= rows || rowB >= rows) return false;

	const type *arow = values[rowA];
	type *brow = values[rowB];

	for (int j : Times{columns}) {
		brow[j] += arow[j] * scalor;
	}

	return true;
}

// behaves oddly but doesn't crash if m_a == m_b
bool QuickMatrix::partialScaleAndAdd(int rowA, int rowB, const type scalor) {
	return partialSAF(rowA, rowB, rowA, scalor);
}

bool QuickMatrix::partialSAF(int rowA, int rowB, int from, const type scalor) {
	if (!values || rowA >= rows || rowB >= rows || from >= columns) return false;

	const type *arow = values[rowA];
	type *brow = values[rowB];

	for (int j : Range{from, columns}) {
		brow[j] += arow[j] * scalor;
	}

	return true;
}

QuickVector QuickMatrix::normalizeRows() const {
	QuickVector ret = QuickVector{columns};

	for (int j : Times{rows}) {
		const type *row = values[j];
		int max_loc = 0;

		for (int i : Times{columns}) {
			if (std::abs(row[max_loc]) < std::abs(row[i])) {
				max_loc = i;
			}
		}

		type scalar = 1.0 / row[max_loc];

		for (int i : Times{columns}) {
			ret[i] = row[i] * scalar;
		}
	}

	return ret;
}

QuickVector QuickMatrix::operator * (const QuickVector &operandvec) const {
	if (operandvec.size() != columns) return QuickVector{};

	QuickVector ret = QuickVector{rows};

	for (int i : Times{rows}) {
		type sum = 0;
		for (int j : Times{columns}) {
			sum += values[i][j] * operandvec[j];
		}
		ret[i] = sum;
	}

	return ret;
}

QuickMatrix & QuickMatrix::operator += (const QuickMatrix &operandmat) {
	if (operandmat.rows != rows || operandmat.columns != columns) return *this;

	for (int i : Times{rows}) {
		for (int j : Times{columns}) {
			values[i][j] += operandmat.values[i][j];
		}
	}

	return *this;
}

QuickMatrix QuickMatrix::operator * (const QuickMatrix &operandmat) const {
	if (operandmat.rows != rows) return QuickMatrix{};

	QuickMatrix ret = QuickMatrix{rows, operandmat.columns};

	for (int i : Times{rows}) {
		for (int j : Times{operandmat.columns}) {
			type sum = 0;
			for (int k : Times{columns}) {
				sum += values[i][k] * operandmat.values[k][j];
			}
			ret.values[i][j] = sum;
		}
	}

	return ret;
}

void QuickMatrix::dumpToAux() const {
	for (int j : Times{rows}) {
		for (int i : Times{columns}) {
			cout << values[j][i] << ' ';
		}
		cout << endl;
	}
}

void QuickMatrix::fillWithZero() {
	for (int j : Times{rows}) {
		memset(values[j], 0, columns * sizeof(type)); // fastest method. =)
	}
}

// sets the diagonal to a constant.
QuickMatrix & QuickMatrix::operator = (const type y) {
	fillWithZero();
	auto size = std::min(columns, rows);

	for (int i : Times{size}) {
		values[i][i] = y;
	}
	return *this;
}

QuickMatrix & QuickMatrix::operator = (QuickMatrix &&othermat) {
	release();

	values = othermat.values;
	rows = othermat.rows;
	columns = othermat.columns;

	othermat.values = nullptr;
	othermat.rows = 0;
	othermat.columns = 0;

	return *this;
}
