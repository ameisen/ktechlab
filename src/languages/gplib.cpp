/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "gplib.h"
#include "languagemanager.h"
#include "logview.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kprocess.h>

Gplib::Gplib(ProcessChain *processChain) :
	ExternalLanguage(
		processChain,
		"Gpasm",
		"*** Archiving successful ***",
		"*** Archiving failed ***"
	)
{}


Gplib::~Gplib()
{
}


void Gplib::processInput(const ProcessOptions &options)
{
	resetLanguageProcess();
	processOptions_ = options;

	*m_languageProcess << ("gplib");
	*m_languageProcess << ("--create");

	*m_languageProcess << ( options.intermediaryOutput() );

	const QStringList inputFiles = options.inputFiles();
	QStringList::const_iterator end = inputFiles.end();
	for ( QStringList::const_iterator it = inputFiles.begin(); it != end; ++it )
		*m_languageProcess << ( *it );

	if ( !start() )
	{
		KMessageBox::sorry( LanguageManager::self()->logView(), i18n("Linking failed. Please check you have gputils installed.") );
		processInitFailed();
		return;
	}
}


bool Gplib::isError( const QString &message ) const
{
	return message.contains( "Error", Qt::CaseInsensitive );
}


bool Gplib::isWarning( const QString &message ) const
{
	return message.contains( "Warning", Qt::CaseInsensitive );
}


MessageInfo Gplib::extractMessageInfo( const QString &text )
{

	if ( text.length()<5 || !text.startsWith("/") )
		return MessageInfo();
#if 0
	const int index = text.indexOf( ".asm", 0, Qt::CaseInsensitive )+4;
	if ( index == -1+4 )
		return MessageInfo();
	const QString fileName = text.left(index);

	// Extra line number
	const QString message = text.right(text.length()-index);
	const int linePos = message.indexOf( QRegExp(":[\\d]+") );
	int line = -1;
	if ( linePos != -1 )
{
		const int linePosEnd = message.indexOf( ':', linePos+1 );
		if ( linePosEnd != -1 )
{
			const QString number = message.mid( linePos+1, linePosEnd-linePos-1 ).trimmed();
			bool ok;
			line = number.toInt(&ok)-1;
			if (!ok) line = -1;
}
}
	return MessageInfo( fileName, line );
#endif
	return MessageInfo();
}




ProcessOptions::Path Gplib::outputPath( ProcessOptions::Path inputPath ) const
{
	switch (inputPath)
	{
		case ProcessOptions::Path::Object_Library:
			return ProcessOptions::Path::None;

		default:
			return ProcessOptions::Path::Invalid;
	}

	return ProcessOptions::Path::Invalid;
}
