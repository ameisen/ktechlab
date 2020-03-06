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
#include "gplib.h"
#include "language.h"
#include "languagemanager.h"
#include "logview.h"
#include "ktechlab.h"
#include "outputmethoddlg.h"
#include "processchain.h"
#include "projectmanager.h"
#include "textdocument.h"

#include "flowcode.h"
#include "gpasm.h"
#include "gpdasm.h"
#include "gplink.h"
#include "microbe.h"
#include "picprogrammer.h"
#include "sdcc.h"

#include <qdebug.h>
#include <klocalizedstring.h>
#include <ktemporaryfile.h>
#include <qfile.h>
#include <qtimer.h>

#include <ktlconfig.h>


//BEGIN class ProcessChain
ProcessChain::ProcessChain( ProcessOptions options, const char *name )
	: QObject( KTechlab::self() /*, name */ )
{
    setObjectName( name );
	m_pFlowCode = 0l;
	m_pGpasm = 0l;
	m_pGpdasm = 0l;
	m_pGplib = 0l;
	m_pGplink = 0l;
	m_pMicrobe = 0l;
	m_pPicProgrammer = 0l;
	m_pSDCC = 0l;
	processOptions_ = options;

	QString target;
	if ( ProcessOptions::to( options.processPath() ) == ProcessOptions::MediaType::Pic )
		target = options.m_picID;
	else
		target = options.targetFile();

	LanguageManager::self()->logView()->addOutput( i18n("Building: %1", target ), LogView::ot_important );
	QTimer::singleShot( 0, this, SLOT(compile()) );
}


ProcessChain::~ProcessChain()
{
	delete m_pFlowCode;
	delete m_pGpasm;
	delete m_pGpdasm;
	delete m_pGplib;
	delete m_pGplink;
	delete m_pMicrobe;
	delete m_pPicProgrammer;
	delete m_pSDCC;
}


// void ProcessChain::compile( ProcessOptions * options )
void ProcessChain::compile()
{
	// If the micro id in the options is empty, then attempt to get it from any
	// open project (it might not be necessarily...but won't hurt if it isn't).
	if ( processOptions_.m_picID.isEmpty() )
	{
		if ( ProjectInfo * projectInfo = ProjectManager::self()->currentProject() )
		{
			ProjectItem * projectItem = projectInfo->findItem( processOptions_.inputFiles().first() );
			if (projectItem)
				processOptions_.m_picID = projectItem->microID();
		}
	}

	switch ( processOptions_.processPath() )
	{
#define DIRECT_PROCESS( path, processor ) \
        case ProcessOptions::path: \
            { \
                processor()->processInput(processOptions_); break; \
            }
#define INDIRECT_PROCESS( path, processor, extension ) \
        case ProcessOptions::path: \
            { \
                KTemporaryFile f; \
                f.setSuffix( extension ); \
                f.open(); \
                f.close(); \
                processOptions_.setIntermediaryOutput( f.fileName() ); \
                processor()->processInput(processOptions_); \
                break; \
            }

		INDIRECT_PROCESS(	Path::AssemblyAbsolute_PIC,			gpasm,		".hex" )
		DIRECT_PROCESS(		Path::AssemblyAbsolute_Program,		gpasm )
		INDIRECT_PROCESS(	Path::AssemblyRelocatable_Library,	gpasm,		".o" )
		DIRECT_PROCESS(		Path::AssemblyRelocatable_Object,		gpasm )
		INDIRECT_PROCESS(	Path::AssemblyRelocatable_PIC,		gpasm,		".o" )
		INDIRECT_PROCESS(	Path::AssemblyRelocatable_Program,	gpasm,		".o" )
		DIRECT_PROCESS(		Path::C_AssemblyRelocatable,			sdcc )
		INDIRECT_PROCESS(	Path::C_Library,						sdcc,		".asm" )
		INDIRECT_PROCESS(	Path::C_Object,						sdcc,		".asm" )
		INDIRECT_PROCESS(	Path::C_PIC,							sdcc,		".asm" )
		INDIRECT_PROCESS(	Path::C_Program,						sdcc,		".asm" )
		INDIRECT_PROCESS(	Path::FlowCode_AssemblyAbsolute,		flowCode,	".microbe" )
		DIRECT_PROCESS(		Path::FlowCode_Microbe,				flowCode )
		INDIRECT_PROCESS(	Path::FlowCode_PIC,					flowCode,	".microbe" )
		INDIRECT_PROCESS(	Path::FlowCode_Program,				flowCode,	".microbe" )
		DIRECT_PROCESS(		Path::Microbe_AssemblyAbsolute,		microbe )
		INDIRECT_PROCESS(	Path::Microbe_PIC,					microbe,	".asm" )
		INDIRECT_PROCESS(	Path::Microbe_Program,				microbe,	".asm" )
		DIRECT_PROCESS(		Path::Object_Disassembly,				gpdasm )
		DIRECT_PROCESS(		Path::Object_Library,					gplib )
		INDIRECT_PROCESS(	Path::Object_PIC,						gplink,		".lib" )
		DIRECT_PROCESS(		Path::Object_Program,					gplink )
		DIRECT_PROCESS(		Path::PIC_AssemblyAbsolute,			picProgrammer )
		DIRECT_PROCESS(		Path::Program_Disassembly,			gpdasm )
		DIRECT_PROCESS(		Path::Program_PIC,					picProgrammer )
#undef DIRECT_PROCESS
#undef INDIRECT_PROCESS

		case ProcessOptions::Path::Invalid:
			qWarning() << Q_FUNC_INFO << "Process path is invalid" << endl;

		case ProcessOptions::Path::None:
			qWarning() << Q_FUNC_INFO << "Nothing to do" << endl;
			break;
	}
}


void ProcessChain::slotFinishedCompile(Language *language)
{
	ProcessOptions options = language->processOptions();

	if ( options.b_addToProject && ProjectManager::self()->currentProject() )
		ProjectManager::self()->currentProject()->addFile( KUrl(options.targetFile()) );

	ProcessOptions::MediaType typeTo = ProcessOptions::to( processOptions_.processPath() );

	TextDocument * editor = 0l;
	if ( KTLConfig::reuseSameViewForOutput() )
	{
		editor = options.textOutputTarget();
		if ( editor && (!editor->url().isEmpty() || editor->isModified()) )
			editor = 0l;
	}

	switch (typeTo)
	{
		case ProcessOptions::MediaType::AssemblyAbsolute:
		case ProcessOptions::MediaType::AssemblyRelocatable:
		case ProcessOptions::MediaType::Disassembly:
		case ProcessOptions::MediaType::Library:
		case ProcessOptions::MediaType::Microbe:
		case ProcessOptions::MediaType::Object:
		case ProcessOptions::MediaType::Program:
		case ProcessOptions::MediaType::C:
		{
			switch ( options.method() )
			{
				case ProcessOptions::Method::LoadAsNew:
				{
					if ( !editor )
						editor = DocManager::self()->createTextDocument();

					if ( !editor )
						break;

					QString text;
					QFile f( options.targetFile() );
					if ( !f.open( QIODevice::ReadOnly ) )
					{
						editor->deleteLater();
						editor = 0l;
						break;
					}

					QTextStream stream(&f);

					while ( !stream.atEnd() )
						text += stream.readLine()+'\n';

					f.close();

					editor->setText( text, true );
					break;
				}

				case ProcessOptions::Method::Load:
				{
					editor = dynamic_cast<TextDocument*>( DocManager::self()->openURL(options.targetFile()) );
					break;
				}

				case ProcessOptions::Method::Forget:
					break;
			}
		}

		case ProcessOptions::MediaType::FlowCode:
		case ProcessOptions::MediaType::Pic:
		case ProcessOptions::MediaType::Unknown:
			break;
	}


	if (editor)
	{
		switch (typeTo)
		{
			case ProcessOptions::MediaType::AssemblyAbsolute:
			{
				if ( KTLConfig::autoFormatMBOutput() )
					editor->formatAssembly();
				editor->slotInitLanguage( TextDocument::ct_asm );
				break;
			}

			case ProcessOptions::MediaType::AssemblyRelocatable:
			case ProcessOptions::MediaType::C:
				editor->slotInitLanguage( TextDocument::ct_c );
				break;

			case ProcessOptions::MediaType::Disassembly:
				break;

			case ProcessOptions::MediaType::Library:
			case ProcessOptions::MediaType::Object:
			case ProcessOptions::MediaType::Program:
				editor->slotInitLanguage( TextDocument::ct_hex );
				break;

			case ProcessOptions::MediaType::Microbe:
				editor->slotInitLanguage( TextDocument::ct_microbe );
				break;

			case ProcessOptions::MediaType::FlowCode:
			case ProcessOptions::MediaType::Pic:
			case ProcessOptions::MediaType::Unknown:
				break;
		}

		DocManager::self()->giveDocumentFocus( editor );
	}

	options.setTextOutputtedTo( editor );

	emit successful(options);
	emit successful();
}

#define LanguageFunction(a,b,c) \
a * ProcessChain::b( ) \
{ \
	if ( !c ) \
	{ \
		c = new a( this ); \
		connect( c, SIGNAL(processSucceeded(Language* )), this, SLOT(slotFinishedCompile(Language* )) ); \
		connect( c, SIGNAL(processFailed(Language* )), this, SIGNAL(failed()) ); \
	} \
	return c; \
}

LanguageFunction( FlowCode, flowCode, m_pFlowCode )
LanguageFunction( Gpasm, gpasm, m_pGpasm )
LanguageFunction( Gpdasm, gpdasm, m_pGpdasm )
LanguageFunction( Gplib, gplib, m_pGplib )
LanguageFunction( Gplink, gplink, m_pGplink )
LanguageFunction( Microbe, microbe, m_pMicrobe )
LanguageFunction( PicProgrammer, picProgrammer, m_pPicProgrammer )
LanguageFunction( SDCC, sdcc, m_pSDCC )
//END class ProcessChain



//BEGIN class ProcessListChain
ProcessListChain::ProcessListChain( ProcessOptionsList pol, const char * name )
	: QObject( KTechlab::self() /*, name  */ )
{
    setObjectName( name );
	m_processOptionsList = pol;

	// Start us off...
	slotProcessChainSuccessful();
}


void ProcessListChain::slotProcessChainSuccessful()
{
	if ( m_processOptionsList.isEmpty() )
	{
		emit successful();
		return;
	}

	ProcessOptionsList::iterator it = m_processOptionsList.begin();
	ProcessOptions po = *it;
	m_processOptionsList.erase(it);

	ProcessChain * pc = LanguageManager::self()->compile(po);

	connect( pc, SIGNAL(successful()), this, SLOT(slotProcessChainSuccessful()) );
	connect( pc, SIGNAL(failed()), this, SLOT(slotProcessChainFailed()) );
}


void ProcessListChain::slotProcessChainFailed()
{
	emit failed();
}
//END class ProcessListChain


#include "moc_processchain.cpp"
