#pragma once

#include "btreenode.h"

/**
Keeps persistant information needed and the algorithm for traversing the binary trees made of BTreeNodes, initialise either by passing a BTreeBase or BTreeNode to traverse a sub tree.

Note that this is designed for traversing in the *reverse* way starting at the end of each branch
in order to calculate the expression contained in the tree.

@author Daniel Clarke
*/
class Traverser
{
public:
	Traverser(BTreeNode *root);
	~Traverser();

	/** Find where to start in the tree and return it also resets all the data related to the traversal. */
	BTreeNode *start();

	/** Finds the next node to move to and returns it. */
	BTreeNode *next();

	/** Returns true if we are on the left branch, false otherwise. */
	bool onLeftBranch();

	/** Returns the node on the opposite branch of the parent. */
	BTreeNode * oppositeNode();

	BTreeNode * current() const { return m_current; }

	void setCurrent(BTreeNode *current){m_current = current;}

	/** From the current position, go down the tree taking a left turn always,
	 and stopping when reaching the left terminal node.
	 */
	void descendLeftwardToTerminal();

	/** It might occur in the future that next() does not just move to the parent,
	 so use this for moving to parent
	 */
	void moveToParent();

	BTreeNode *root() const {return m_root;}

protected:
	BTreeNode *m_root;
	BTreeNode *m_current;
};
