#pragma once

#include <QString>
#include <QList>

class PortPin;
typedef QList<PortPin> PortPinList;

/**
@author Daniel Clarke
@author David Saxton
*/
class Variable
{
	public:
		enum VariableType
		{
			charType, // 8 bit file register
			sevenSegmentType, // A pin configuration for a seven segment is represented by a write-only variable.
			keypadType, // A pin configuration for a keypad has 4 rows and n columns (typically n = 3 or 4) - a read-only variable
			invalidType
		};

		Variable( VariableType type, const QString & name );
		Variable();
		~Variable();

		VariableType type() const { return m_type; }
		QString name() const { return m_name; }

		/**
		 * @returns whether the variable can be read from (e.g. the seven
		 * segment variable cannot).
		 */
		bool isReadable() const;
		/**
		 * @returns whether the variable can be written to (e.g. the keypad
		 * variable cannot).
		 */
		bool isWritable() const;
		/**
		 * @see portPinList
		 */
		void setPortPinList( const PortPinList & portPinList );
		/**
		 * Used in seven-segments and keypads,
		 */
		PortPinList portPinList() const { return m_portPinList; }

	protected:
		VariableType m_type;
		QString m_name;
		PortPinList m_portPinList;
};
typedef QList<Variable> VariableList;
