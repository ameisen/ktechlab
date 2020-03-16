#pragma once

#include "quickvector.h"

#include <memory>

class QuickMatrix {
public:
	using type = double;

	QuickMatrix() = default;
	QuickMatrix(int rows, int columns);
	QuickMatrix(int rows);
	QuickMatrix(QuickMatrix &&old);
	QuickMatrix(const QuickMatrix &old);
	QuickMatrix(const QuickMatrix *old);
	~QuickMatrix();

	type at(int row, int column) const;
	bool atPut(int row, int column, const type val);
	bool atAdd(int row, int column, const type val); // just give a negative val to subtract. =)

	bool isSquare() const;

	type *& operator [] (const int row) { return values[row]; }
	const type * operator [] ( const int row) const { return values[row]; }

	int numRows() const { return rows; };
	int numColumns() const { return columns; };

	bool scaleRow(int row, const type scalor);
	bool addRowToRow(int rowA, int rowB);
	bool scaleAndAdd(int rowA, int rowB, const type scalor);
	bool partialScaleAndAdd(int rowA, int rowB, const type scalor);
	bool partialSAF(int rowA, int rowB, int from, const type scalor);
	bool swapRows(int rowA, int rowB);

// functions that accelerate certain types of
// operations that would otherwise require millions of at()'s
	type multstep(int row, int pos, int col) const;
	type multRowCol(int row, int col, int lim) const;

	QuickMatrix transposeSquare() const; // Multiplies self by transpose.
	QuickVector transposeMult(const QuickVector &operandvec) const;

// utility functions:
	void fillWithZero();
	type rowsum(int row) const;
	type absrowsum(int row) const;
	QuickVector normalizeRows() const;

	QuickMatrix & operator = (QuickMatrix &&othermat);

// Matrix arithmetic.
	QuickMatrix &operator += (const QuickMatrix &othermat);
	QuickMatrix &operator *= (const type y);
	QuickMatrix &operator = (const type y); // sets the diagonal to a constant.
//	QuickMatrix *operator =(const QuickMatrix *oldmat);
	QuickMatrix operator * (const QuickMatrix &operandmat) const;

	QuickVector operator * (const QuickVector &operandvec) const;

// debugging
	void dumpToAux() const;

private :
	void release();
	type ** allocate() const;

	type **values = nullptr;
	int rows = 0;
	int columns = 0;
};
