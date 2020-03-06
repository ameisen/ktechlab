/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "language.h"
#include "microselectwidget.h"
#include "picprogrammer.h"
#include "port.h"
#include "programmerdlg.h"
#include "ktlconfig.h"
#include "ui_programmerwidget.h"


#include <qcombobox.h>

#include <kguiitem.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>


class ProgrammerWidget : public QWidget, public Ui::ProgrammerWidget {
    public:
    ProgrammerWidget(QWidget *parent) : QWidget(parent) {
        setupUi(this);
    }
};

ProgrammerDlg::ProgrammerDlg( const QString & picID, QWidget *parent, const char *name )
	: KDialog( parent ) // , name, true, i18n("PIC Programmer"), Ok|Cancel )
{
    setObjectName(name);
    setModal(true);
    setCaption(i18n("PIC Programmer"));
    setButtons(Ok|Cancel);

	// Change the "Ok" button to a "Burn" button
	KGuiItem burnItem = KStandardGuiItem::ok();
	burnItem.setText( i18n("Burn") );
	//setButtonOK( burnItem );
    setButtonGuiItem(Ok, burnItem);


	m_bAccepted = false;
	m_pProgrammerWidget = new ProgrammerWidget( this );
	m_pProgrammerSettings = new PicProgrammerSettings;

	// Setup the list of programmers
	QComboBox * programmerCombo = m_pProgrammerWidget->m_pProgrammerProgram;
	QStringList programmerNames = m_pProgrammerSettings->configNames( false );
	programmerCombo->insertItems(programmerCombo->count(), programmerNames );
	//programmerCombo->setSizeLimit( programmerNames.size() );
    programmerCombo->setMaxCount( programmerNames.size() );
	//programmerCombo->setCurrentText( KTLConfig::picProgrammerProgram() ); // 2018.12.07
    {
        QComboBox *c = programmerCombo;
        QString text = KTLConfig::picProgrammerProgram();
        int i = c->findText(text);
        if (i != -1)
            c->setCurrentIndex(i);
        else if (c->isEditable())
            c->setEditText(text);
        else
            c->setItemText(c->currentIndex(), text);
    }
	// Sets up the list of ports
	m_pProgrammerWidget->m_pPicProgrammerPort->insertItems(
        m_pProgrammerWidget->m_pPicProgrammerPort->count(),
        Port::ports( Port::ExistsAndRW ) );
	//m_pProgrammerWidget->m_pPicProgrammerPort->setCurrentText( KTLConfig::picProgrammerPort() ); // 2018.12.07
    {
        QComboBox *c = m_pProgrammerWidget->m_pPicProgrammerPort;
        QString text = KTLConfig::picProgrammerPort();
        int i = c->findText(text);
        if (i != -1)
            c->setCurrentIndex(i);
        else if (c->isEditable())
            c->setEditText(text);
        else
            c->setItemText(c->currentIndex(), text);
    }

	// Set the pic type to the one requested
	if ( !picID.isEmpty() )
		m_pProgrammerWidget->m_pMicroSelect->setMicro( picID );

	setMainWidget( m_pProgrammerWidget );
}


ProgrammerDlg::~ProgrammerDlg()
{
}


void ProgrammerDlg::initOptions( ProcessOptions * options )
{
	if ( !options )
		return;

	options->m_picID = m_pProgrammerWidget->m_pMicroSelect->micro();
	options->m_port = m_pProgrammerWidget->m_pPicProgrammerPort->currentText();
	options->m_program = m_pProgrammerWidget->m_pProgrammerProgram->currentText();
}


void ProgrammerDlg::accept()
{
	m_bAccepted = true;
	hide();
}


void ProgrammerDlg::reject()
{
    hide();
	m_bAccepted = false;
}


MicroSelectWidget * ProgrammerDlg::microSelect( ) const
{
	return m_pProgrammerWidget->m_pMicroSelect;
}


#include "moc_programmerdlg.cpp"
