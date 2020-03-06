#pragma once

#include "microbe.h"

#include <QString>

class PIC14;
class BTreeNode;
class Microbe;

/**
@author Daniel Clarke
@author David Saxton
*/
class Expression
{
	public:
		enum Operation
		{
			noop,
			addition,
			subtraction,
			multiplication,
			division,
			exponent,
			equals,
			notequals,
			pin,//(returns the truth value obtatined by testing the pin)
			notpin, //(the result of doing the pin op NOTted).]
			read_keypad, //read value from keypad
			function,
			bwand,
			bwor,
			bwxor,
			bwnot,
			divbyzero, // used to make handling this situation easier
			gt,
			lt,
			ge,
			le
		};

		Expression(PIC14 *pic, Microbe *master, SourceLine sourceLine, bool supressNumberTooBig );
		~Expression();

		/**
		 * Generates the code needed to evaluate an expression. Firstly, a tree
		 * is generated from the expression string; then that tree is traversed
		 * to generate the assembly.
		 */
		void compileExpression( const QString & expression);
		void compileConditional( const QString & expression, Code * ifCode, Code * elseCode );
		/**
		 * Returns a *number* rather than evaluating code, and sets isConstant to true
		 * if it the expression evaluated to a constant.
		 */
		QString processConstant( const QString & expr, bool * isConsant );

	private:
		PIC14 *m_pic;
		Microbe *mb;

		/** Turns the operations encoded in the given tree into assembly code */
		void traverseTree( BTreeNode *root, bool conditionalRoot = false );

		bool isUnaryOp(Operation op);

		void expressionValue( QString expression, BTreeBase *tree, BTreeNode *node );
		void doOp( Operation op, BTreeNode *left, BTreeNode *right );
		void doUnaryOp( Operation op, BTreeNode *node );
		/**
		 * Parses an expression, and generates a tree structure from it.
		 */
		void buildTree( const QString & expression, BTreeBase *tree, BTreeNode *node, int level );

		static int findSkipBrackets( const QString & expr, char ch, int startPos = 0);
		static int findSkipBrackets( const QString & expr, QString phrase, int startPos = 0);

		QString stripBrackets( QString expression );

		void mistake( Microbe::MistakeType type, const QString & context = 0 );

		SourceLine m_sourceLine;

		Code * m_ifCode;
		Code * m_elseCode;

		/**
		 *Returns expression type
		 * 0 = directly usable number (literal)
		 * 1 = variable
		 * 2 = expression that needs evaluating
		 * (maybe not, see enum).
		 */
		ExprType expressionType( const QString & expression );
		static bool isLiteral( const QString &text );
		/**
		 * Normally, only allow numbers upto 255; but for some uses where the
		 * number is not going to be placed in a PIC register (such as when
		 * delaying), we can ignore numbers being too big.
		 */
		bool m_bSupressNumberTooBig;
};
