/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "asminfo.h"
#include "languagemanager.h"
#include "logview.h"
#include "microinfo.h"
#include "microlibrary.h"
#include "sdcc.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kprocess.h>

#include <ktlconfig.h>

SDCC::SDCC(ProcessChain * processChain) :
	ExternalLanguage(
		processChain,
		"SDCC",
		"*** Compilation successful ***",
		"*** Compilation failed ***"
	)
{}

SDCC::~SDCC()
{
}

void SDCC::processInput(const ProcessOptions &options)
{
	resetLanguageProcess();

	MicroInfo * info = MicroLibrary::self()->microInfoWithID( options.m_picID );
	if (!info)
	{
		outputError( i18n("Could not find PIC with ID \"%1\".", options.m_picID) );
		return;
	}

	processOptions_ = options;

    if ( KTLConfig::sDCC_install_prefix().isEmpty()) {
        qDebug() << Q_FUNC_INFO << "using system sdcc";
        *m_languageProcess << ("sdcc");
    } else {
        qDebug() << Q_FUNC_INFO << "using sdcc at " << KTLConfig::sDCC_install_prefix();
        *m_languageProcess << ( KTLConfig::sDCC_install_prefix().append("/bin/sdcc") );
    }

	//BEGIN Pass custom sdcc options
#define ARG(text,option) if ( KTLConfig::text() ) *m_languageProcess << ( QString("--%1").arg(option) );
	// General
	ARG( sDCC_nostdlib,			"nostdlib" )
	ARG( sDCC_nostdinc,			"nostdinc" )
	ARG( sDCC_less_pedantic,		"less-pedantic" )
	ARG( sDCC_std_c89,			"std-c89" )
	ARG( sDCC_std_c99,			"std-c99" )
    ARG( sDCC_use_non_free,   "use-non-free" );

	// Code generation
	ARG( sDCC_stack_auto,			"stack-auto" )
	ARG( sDCC_int_long_reent,		"int-long-reent" )
	ARG( sDCC_float_reent,			"float-reent" )
	ARG( sDCC_fommit_frame_pointer,	"fommit-frame-pointer" )
	ARG( sDCC_no_xinit_opt,			"no-xinit-opt" )
	ARG( sDCC_all_callee_saves,		"all-callee-saves" )

	// Optimization
	ARG( sDCC_nooverlay,			"nooverlay" )
	ARG( sDCC_nogcse,			"nogcse" )
	ARG( sDCC_nolabelopt,			"nolabelopt" )
	ARG( sDCC_noinvariant,			"noinvariant" )
	ARG( sDCC_noinduction,			"noinduction" )
	ARG( sDCC_no_peep,			"no-peep" )
	ARG( sDCC_noloopreverse,		"noloopreverse" )
	ARG( sDCC_opt_code_size,		"opt-code-size" )
	ARG( sDCC_opt_code_speed,		"opt-code-speed" )
	ARG( sDCC_peep_asm,			"peep-asm" )
	ARG( sDCC_nojtbound,			"nojtbound" )

	// PIC16 Specific
	if ( info->instructionSet()->set() == AsmInfo::PIC16 )
	{
		ARG( sDCC_nodefaultlibs,		"nodefaultlibs" )
		ARG( sDCC_pno_banksel,			"pno-banksel" )
		ARG( sDCC_pstack_model_large,	"pstack-model=large" )
		ARG( sDCC_debug_xtra,			"debug-xtra" )
		ARG( sDCC_denable_peeps,		"denable-peeps" )
		ARG( sDCC_calltree,			"calltree" )
		ARG( sDCC_fstack,			"fstack" )
		ARG( sDCC_optimize_goto,		"optimize-goto" )
		ARG( sDCC_optimize_cmp,			"optimize-cmp" )
		ARG( sDCC_optimize_df,			"optimize-df" )
	}
#undef ARG

    if ( !KTLConfig::sDCC_install_prefix().isEmpty()) {
        QString incDir="";
        switch (info->instructionSet()->set()) {
            case AsmInfo::PIC14: incDir = "pic14"; break;
            case AsmInfo::PIC16: incDir = "pic16"; break;
            default:
                qWarning() << Q_FUNC_INFO << "unsupported PIC instruction set " << info->instructionSet()->set();
        }
        *m_languageProcess << ( QString("-I%1/share/sdcc/include").arg(KTLConfig::sDCC_install_prefix()) );
        *m_languageProcess << ( QString("-I%1/share/sdcc/include/%2").arg(KTLConfig::sDCC_install_prefix()).arg(incDir) );
        *m_languageProcess << ( QString("-I%1/share/sdcc/non-free/include/%2").arg(KTLConfig::sDCC_install_prefix()).arg(incDir) );
    }

	if ( !KTLConfig::miscSDCCOptions().isEmpty() ) {
        // note: this will not work with quotes inside the text; those need special parsing
		*m_languageProcess << ( KTLConfig::miscSDCCOptions().split(QRegExp(" ")) );
    }
	//END Pass custom sdcc options


	*m_languageProcess << ("--debug"); // Enable debugging symbol output
	*m_languageProcess << ("-S"); // Compile only; do not assemble or link

	QString asmSwitch;
	switch ( info->instructionSet()->set() )
	{
		case AsmInfo::PIC12:
			// Last time I checked, SDCC doesn't support Pic12, and probably never will, but whatever...
			asmSwitch = "-mpic12";
			break;
		case AsmInfo::PIC14:
			asmSwitch = "-mpic14";
			break;
		case AsmInfo::PIC16:
			asmSwitch = "-mpic16";
			break;
	}

	*m_languageProcess << (asmSwitch);

	*m_languageProcess << ( "-"+options.m_picID.toLower() );

	*m_languageProcess << ( options.inputFiles().first() );

	*m_languageProcess << ("-o");
	*m_languageProcess << ( options.intermediaryOutput() );

	if ( !start() )
	{
		KMessageBox::sorry( LanguageManager::self()->logView(), i18n("Compilation failed. Please check you have sdcc installed.") );
		processInitFailed();
		return;
	}
}


bool SDCC::isError( const QString &message ) const
{
	if ( message.startsWith("Error:") )
		return true;

	return false;
}

bool SDCC::isStderrOutputFatal( const QString & message ) const
{
    qDebug() << Q_FUNC_INFO << "message=" << message;
	if ( message.startsWith("Processor:") )
		return false;

    return false; // note: return code from SDCC will tell if anything is fatal
	//return true; // 2017.06.18 - by default stderr messages are not fatal
}

bool SDCC::isWarning( const QString &message ) const
{
	if ( message.startsWith("Warning:") )
		return true;
	return false;
}

ProcessOptions::Path SDCC::outputPath( ProcessOptions::Path inputPath ) const
{
	switch (inputPath)
	{
		case ProcessOptions::Path::AssemblyRelocatable_Library:
			return ProcessOptions::Path::AssemblyRelocatable_Library;

		case ProcessOptions::Path::AssemblyRelocatable_Object:
			return ProcessOptions::Path::AssemblyRelocatable_Object;

		case ProcessOptions::Path::AssemblyRelocatable_PIC:
			return ProcessOptions::Path::AssemblyAbsolute_PIC;

		case ProcessOptions::Path::AssemblyRelocatable_Program:
			return ProcessOptions::Path::AssemblyRelocatable_Program;

		default:
			return ProcessOptions::Path::Invalid;
	}

	return ProcessOptions::Path::Invalid;
}
