#pragma once

#include "btreebase.h"
#include "expression.h"

#include <QString>
#include <QList>

/**
A node points to the two child nodes (left and right), and contains the binary
operation used to combine them.

@author Daniel Clarke
@author David Saxton
*/
class BTreeNode
{
	public:
		BTreeNode();
		BTreeNode(BTreeNode *p, BTreeNode *l, BTreeNode *r);
		~BTreeNode();

		/**
		 * Used for debugging purposes; prints the tree structure to stdout.
		 */
// 		void printTree();
		/**
		 * Recursively delete all children of a node.
		 */
		void deleteChildren();
		/**
		 * @return the parent node.
		 */
		BTreeNode *parent() const { return m_parent; }
		/**
		 * @return the left child node.
		 */
		BTreeNode *left() const { return m_left; }
		/**
		 * @return the right child node.
		 */
		BTreeNode *right() const { return m_right; }
		void setParent(BTreeNode *parent) { m_parent = parent; }
		/**
		 * Set the child node on the left to the one give, and reparents it to
		 * this node.
		 */
		void setLeft(BTreeNode *left) { m_left = left; m_left->setParent( this ); }
		/**
		 * Set the child node on the right to the one give, and reparents it to
		 * this node.
		 */
		void setRight(BTreeNode *right) { m_right = right; m_right->setParent( this ); }
		/**
		 * @return true if have a left or a right child node.
		 */
		bool hasChildren() const { return m_left || m_right; }

		ExprType type() const {return m_type;}
		void setType(ExprType type) { m_type = type; }
		QString value() const {return m_value;}
		void setValue( const QString & value ) { m_value = value; }

		Expression::Operation childOp() const {return m_childOp;}
		void setChildOp(Expression::Operation op){ m_childOp = op;}

		void setReg( const QString & r ){ m_reg = r; }
		QString reg() const {return m_reg;}

		bool needsEvaluating() const { return hasChildren(); }

	protected:
		BTreeNode *m_parent;
		BTreeNode *m_left;
		BTreeNode *m_right;

		/** This is used to remember what working register contains the value of the node during assembly.*/
		QString m_reg;

		ExprType m_type;
		QString m_value;

		Expression::Operation m_childOp;
};
