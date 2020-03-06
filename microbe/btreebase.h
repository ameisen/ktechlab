#pragma once

#include "microbe.h"
#include "btreenode.h"

/**
@short This holds a pointer to the start of the tree, and provides the traversal code.
@author Daniel Clarke
*/
class BTreeBase{
public:
    BTreeBase() = default;
    ~BTreeBase();

    /** Return a pointer to the root node of the tree */
    BTreeNode * root() const { return m_root; }

    /** Set the root node of the tree */
    void setRoot(BTreeNode *root) { m_root = root; }

    /** Link the node into the tree. a.t.m all this really
    does it sets the parent/child relationship pointers,
    but is used in case something needs to be changed in the future
    Added to the left if left == true or the right if left == false */
    void addNode(BTreeNode *parent, BTreeNode *node, bool left);

	/** Tidies the tree up; merging constants and removing redundant branches */
    void pruneTree(BTreeNode *root, bool conditionalRoot = true);

    /** Put a node in place of another, linking it correctly into the parent. */
    void replaceNode(BTreeNode *node, BTreeNode *replacement);

protected:
    BTreeNode *m_root = nullptr;
};
