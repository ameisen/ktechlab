/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "contexthelp.h"
#include "docmanager.h"
#include "logview.h"
#include "microbe.h"
#include "languagemanager.h"

#include <qdebug.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include <qfile.h>
#include <kprocess.h>

Microbe::Microbe(ProcessChain *processChain) :
	ExternalLanguage(
		processChain,
		"Microbe",
		"*** Compilation failed ***",
		"*** Compilation successful ***"
	)
{
#if 0
	// Setup error messages list
	QFile file( locate("appdata",i1 8n("error_messages_en_gb")) );
	if ( file.open( QIODevice::ReadOnly ) )
	{
        QTextStream stream( &file );
        QString line;
        while ( !stream.atEnd() )
		{
			line = stream.readLine(); // line of text excluding '\n'
			if ( !line.isEmpty() )
			{
				bool ok;
				const int pos = line.left( line.indexOf("#") ).toInt(&ok);
				if (ok) {
					m_errorMessages[pos] = line.right(line.length()-line.indexOf("#"));
				} else {
					qCritical() << Q_FUNC_INFO << "Error parsing Microbe error-message file"<<endl;
				}
			}
        }
		file.close();
	}
#endif
}

Microbe::~Microbe()
{
}


void Microbe::processInput(const ProcessOptions &options)
{
	resetLanguageProcess();
	processOptions_ = options;

	*m_languageProcess << ("microbe");

	// Input Asm file
	*m_languageProcess << ( options.inputFiles().first() );

	// Output filename
	*m_languageProcess << ( options.intermediaryOutput() );

	*m_languageProcess << ("--show-source");

	if ( !start() )
	{
		KMessageBox::sorry( LanguageManager::self()->logView(), i18n("Assembly failed. Please check you have KTechlab installed properly (\"microbe\" could not be started).") );
		processInitFailed();
		return;
	}
}


bool Microbe::isError( const QString &message ) const
{
	 return message.contains( "Error", Qt::CaseInsensitive );
}

bool Microbe::isWarning( const QString &message ) const
{
	return message.contains( "Warning", Qt::CaseInsensitive );
}


ProcessOptions::Path Microbe::outputPath( ProcessOptions::Path inputPath ) const
{
	switch (inputPath)
	{
		case ProcessOptions::Path::Microbe_AssemblyAbsolute:
			return ProcessOptions::Path::None;

		case ProcessOptions::Path::Microbe_PIC:
			return ProcessOptions::Path::AssemblyAbsolute_PIC;

		case ProcessOptions::Path::Microbe_Program:
			return ProcessOptions::Path::AssemblyAbsolute_Program;

		default:
			return ProcessOptions::Path::Invalid;
	}

	return ProcessOptions::Path::Invalid;
}
