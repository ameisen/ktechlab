#include "pic14.h"
#include "variable.h"

Variable::Variable( VariableType type, const QString & name )
{
	m_type = type;
	m_name = name;
}


Variable::Variable()
{
	m_type = invalidType;
}


Variable::~Variable()
{
}


void Variable::setPortPinList( const PortPinList & portPinList )
{
	m_portPinList = portPinList;
}


bool Variable::isReadable() const
{
	switch (m_type)
	{
		case charType:
		case keypadType:
			return true;
		case sevenSegmentType:
		case invalidType:
			return false;
	}

	return false;
}


bool Variable::isWritable() const
{
	switch (m_type)
	{
		case charType:
		case sevenSegmentType:
			return true;
		case keypadType:
		case invalidType:
			return false;
	}

	return false;
}
