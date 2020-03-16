/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "asmparser.h"
#include "docmanager.h"
#include "gpasm.h"
#include "logview.h"
#include "languagemanager.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <qregexp.h>

#include <ktlconfig.h>

Gpasm::Gpasm(ProcessChain *processChain) :
	ExternalLanguage(
		processChain,
		"Gpasm",
		"*** Assembly successful ***",
		"*** Assembly failed ***"
	)
{}


Gpasm::~Gpasm()
{
}


void Gpasm::processInput(const ProcessOptions &options)
{
	resetLanguageProcess();
	processOptions_ = options;

	AsmParser p( options.inputFiles().first() );
	p.parse();

	*m_languageProcess << ("gpasm");

	if ( ProcessOptions::from( options.processPath() ) == ProcessOptions::MediaType::AssemblyRelocatable )
		*m_languageProcess << ("--object");

// 	*m_languageProcess << ("--debug-info"); // Debug info

	// Output filename
	*m_languageProcess << ("--output");
	*m_languageProcess << ( options.intermediaryOutput() );

	if ( !options.m_hexFormat.isEmpty() )
	{
		*m_languageProcess << ("--hex-format");
		*m_languageProcess << (options.m_hexFormat);
	}

	// Radix
	if ( !p.containsRadix() )
	{
		*m_languageProcess << ("--radix");
		switch( KTLConfig::radix() )
		{
			case KTLConfig::EnumRadix::Binary:
				*m_languageProcess << ("BIN");
				break;
			case KTLConfig::EnumRadix::Octal:
				*m_languageProcess << ("OCT");
				break;
			case KTLConfig::EnumRadix::Hexadecimal:
				*m_languageProcess << ("HEX");
				break;
			case KTLConfig::EnumRadix::Decimal:
			default:
				*m_languageProcess << ("DEC");
				break;
		}
	}

	// Warning Level
	*m_languageProcess << ("--warning");
	switch( KTLConfig::gpasmWarningLevel() )
	{
		case KTLConfig::EnumGpasmWarningLevel::Warnings:
			*m_languageProcess << ("1");
			break;
		case KTLConfig::EnumGpasmWarningLevel::Errors:
			*m_languageProcess << ("2");
			break;
		default:
		case KTLConfig::EnumGpasmWarningLevel::All:
			*m_languageProcess << ("0");
			break;
	}

	// Ignore case
	if ( KTLConfig::ignoreCase() )
		*m_languageProcess << ("--ignore-case");

	// Dos formatting
	if ( KTLConfig::dosFormat() )
		*m_languageProcess << ("--dos");

	// Force list
	if ( options.b_forceList )
		*m_languageProcess << ("--force-list");

	// Other options
	if ( !KTLConfig::miscGpasmOptions().isEmpty() )
		*m_languageProcess << ( KTLConfig::miscGpasmOptions() );

	// Input Asm file
	*m_languageProcess << ( options.inputFiles().first() );

	if ( !start() )
	{
		KMessageBox::sorry( LanguageManager::self()->logView(), i18n("Assembly failed. Please check you have gputils installed.") );
		processInitFailed();
		return;
	}
}


bool Gpasm::isError( const QString &message ) const
{
	return message.contains( "Error", Qt::CaseInsensitive );
}


bool Gpasm::isWarning( const QString &message ) const
{
	return message.contains( "Warning", Qt::CaseInsensitive );
}


ProcessOptions::Path Gpasm::outputPath( ProcessOptions::Path inputPath ) const
{
	switch (inputPath)
	{
		case ProcessOptions::Path::AssemblyAbsolute_PIC:
			return ProcessOptions::Path::Program_PIC;

		case ProcessOptions::Path::AssemblyAbsolute_Program:
			return ProcessOptions::Path::None;

		case ProcessOptions::Path::AssemblyRelocatable_Library:
			return ProcessOptions::Path::Object_Library;

		case ProcessOptions::Path::AssemblyRelocatable_Object:
			return ProcessOptions::Path::None;

		case ProcessOptions::Path::AssemblyRelocatable_PIC:
			return ProcessOptions::Path::Object_PIC;

		case ProcessOptions::Path::AssemblyRelocatable_Program:
			return ProcessOptions::Path::Object_Program;

		default:
			return ProcessOptions::Path::Invalid;
	}

	return ProcessOptions::Path::Invalid;
}
