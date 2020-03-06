#include "btreenode.h"
#include "pic14.h"

BTreeNode::BTreeNode()
{
	m_parent = 0L;
	m_left = 0L;
	m_right = 0L;
	m_type = unset;
}

BTreeNode::BTreeNode(BTreeNode *p, BTreeNode *l, BTreeNode *r)
{
	m_parent = p;
	m_left = l;
	m_right = r;
}

BTreeNode::~BTreeNode()
{
	// Must not delete children as might be unlinking!!! deleteChildren();
}

void BTreeNode::deleteChildren()
{
	if(m_left)
	{
		m_left->deleteChildren();
		delete m_left;
	}
	if(m_right)
	{
		m_right->deleteChildren();
		delete m_right;
	}

	m_left = 0L;
	m_right = 0L;

	return;
}

// void BTreeNode::printTree()
// {
//
// }
