/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#define protected public
#include <kxmlguiclient.h>
#undef protected

#include "asmformatter.h"
#include "config.h"
#include "filemetainfo.h"
#include "gpsimprocessor.h"
#include "ktechlab.h"
#include "symbolviewer.h"
#include "textdocument.h"
#include "textview.h"
#include "variablelabel.h"
#include "viewiface.h"
#include "ktlfindqobjectchild.h"

//#include <ktexteditor/editinterface.h> // ?
#include <ktexteditor/texthintinterface.h>

// #include "kateview.h"
#include <kaction.h>
#include <kdebug.h>
#include <qdebug.h>
#include <kiconloader.h>
#include <kicon.h>
#include <klocalizedstring.h>
#include <KActionCollection>
// #include <k3popupmenu.h>
#include <kstandarddirs.h>
#include <ktoolbarpopupaction.h>
#include <kmenu.h>
#include <kxmlguifactory.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <qactiongroup.h>

#include <qapplication.h>
#include <qboxlayout.h>
#include <qcursor.h>
//#include <qobjectlist.h>
#include <qtimer.h>
#include <qsharedpointer.h>
#include <qclipboard.h>


//BEGIN class TextView
TextView::TextView( TextDocument * textDocument, ViewContainer *viewContainer, uint viewAreaId, const char *name )
	: View( textDocument, viewContainer, viewAreaId, name )
{
	m_view = textDocument->createKateView(this);
	m_view->insertChildClient(this);

    KActionCollection * ac = actionCollection();

	//BEGIN Convert To * Actions
	//KToolBarPopupAction * pa = new KToolBarPopupAction( i18n("Convert to"), "fork", 0, 0, 0, ac, "program_convert" );
    KToolBarPopupAction * pa = new KToolBarPopupAction( QIcon::fromTheme("fork"), i18n("Convert To"), ac);
    pa->setObjectName("program_convert");
	pa->setDelayed(false);
    ac->addAction(pa->objectName(), pa);

	QMenu * m = pa->menu();

    m->setTitle( i18n("Convert To") );
    QAction *actToMicrobe = m->addAction( QIcon::fromTheme( "convert_to_microbe" ), i18n("Microbe"));
    actToMicrobe->setData( TextDocument::MicrobeOutput );
	m->addAction( QIcon::fromTheme( "convert_to_assembly" ), i18n("Assembly"))->setData( TextDocument::AssemblyOutput );
	m->addAction( QIcon::fromTheme( "convert_to_hex" ), i18n("Hex"))->setData( TextDocument::HexOutput );
	m->addAction( QIcon::fromTheme( "convert_to_pic" ), i18n("PIC (upload)"))->setData( TextDocument::PICOutput );
	connect( m, SIGNAL(triggered(QAction*)), textDocument, SLOT(slotConvertTo(QAction*)) );

	//m->setItemEnabled( TextDocument::MicrobeOutput, false ); // 2018.12.02
    actToMicrobe->setEnabled(false);
    ac->addAction(pa->objectName(), pa);
	//END Convert To * Actions

    {
	//new QAction( i18n("Format Assembly Code"), "", Qt::Key_F12, textDocument, SLOT(formatAssembly()), ac, "format_asm" );
        QAction *action = new QAction( QIcon::fromTheme(""), i18n("Format Assembly Code"), ac);
        action->setObjectName("format_asm");
        action->setShortcut(Qt::Key_F12);
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(formatAssembly()));
        ac->addAction(action->objectName(), action);
    }


#ifndef NO_GPSIM
	//BEGIN Debug Actions
	{
	//new QAction( i18n("Set &Breakpoint"), 0, 0, this, SLOT(toggleBreakpoint()), ac, "debug_toggle_breakpoint" );
        QAction *action = new QAction( QIcon::fromTheme(""), i18n("Set &Breakpoint"), ac);
        action->setObjectName("debug_toggle_breakpoint");
        connect(action, SIGNAL(triggered(bool)), this, SLOT(toggleBreakpoint()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Run"), "debug-run", 0, textDocument, SLOT(debugRun()), ac, "debug_run" );
        QAction *action = new QAction( QIcon::fromTheme("debug-run"), i18n("Run"), ac);
        action->setObjectName("debug_run");
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugRun()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Interrupt"), "media-playback-pause", 0, textDocument, SLOT(debugInterrupt()), ac, "debug_interrupt" );
        QAction *action = new QAction( QIcon::fromTheme("media-playback-pause"), i18n("Interrupt"), ac);
        action->setObjectName("debug_interrupt");
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugInterrupt()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Stop"), "process-stop", 0, textDocument, SLOT(debugStop()), ac, "debug_stop" );
        QAction *action = new QAction( QIcon::fromTheme("process-stop"), i18n("Stop"), ac);
        action->setObjectName("debug_stop");
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugStop()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Step"), "debug-step-instruction", Qt::CTRL|Qt::ALT|Qt::Key_Right, textDocument, SLOT(debugStep()), ac, "debug_step" );
        QAction *action = new QAction( QIcon::fromTheme("debug-step-instruction"), i18n("Step"), ac);
        action->setObjectName("debug_step");
        action->setShortcut(Qt::CTRL|Qt::ALT|Qt::Key_Right);
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugStep()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Step Over"), "debug-step-over", 0, textDocument, SLOT(debugStepOver()), ac, "debug_step_over" );
        QAction *action = new QAction( QIcon::fromTheme("debug-step-over"), i18n("Step Over"), ac);
        action->setObjectName("debug_step_over");
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugStepOver()));
        ac->addAction(action->objectName(), action);
    }
    {
	//new QAction( i18n("Step Out"), "debug-step-out", 0, textDocument, SLOT(debugStepOut()), ac, "debug_step_out" );
        QAction *action = new QAction( QIcon::fromTheme("debug-step-out"), i18n("Step Out"), ac);
        action->setObjectName("debug_step_out");
        connect(action, SIGNAL(triggered(bool)), textDocument, SLOT(debugStepOut()));
        ac->addAction(action->objectName(), action);
    }
	//END Debug Actions
#endif


	setXMLFile( "ktechlabtextui.rc" );
	m_view->setXMLFile( KStandardDirs::locate( "appdata", "ktechlabkateui.rc" ) );

	m_savedCursorLine = 0;
	m_savedCursorColumn = 0;
	m_pViewIface = new TextViewIface(this);

	setAcceptDrops(true);

	m_statusBar->insertItem( "", ViewStatusBar::LineCol );

	//m_view->installPopup( static_cast<Q3PopupMenu*>( KTechlab::self()->factory()->container( "ktexteditor_popup", KTechlab::self() ) ) );
    m_view->setContextMenu( static_cast<QMenu*>( KTechlab::self()->factory()->container( "ktexteditor_popup", KTechlab::self() ) ) );

	//QWidget * internalView = static_cast<QWidget*>( m_view->child( 0, "KateViewInternal" ) ); // 2018.12.02
    QWidget * internalView = static_cast<QWidget*>( ktlFindQObjectChild( m_view, "KateViewInternal" ) );

	connect( m_view, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor &)),	this, SLOT(slotCursorPositionChanged()) );
	connect( m_view, SIGNAL(selectionChanged(KTextEditor::View *)), this, SLOT(slotSelectionmChanged()) );

	setFocusWidget( internalView );
	connect( this, SIGNAL(focused( View* )), this, SLOT(gotFocus()) );

	m_layout->insertWidget( 0, m_view );

	slotCursorPositionChanged();
	slotInitDebugActions();
	initCodeActions();

#ifndef NO_GPSIM
	m_pTextViewLabel = new VariableLabel( this );
	m_pTextViewLabel->hide();

	TextViewEventFilter * eventFilter = new TextViewEventFilter( this );
	connect( eventFilter, SIGNAL(wordHoveredOver( const QString&, int, int )), this, SLOT(slotWordHoveredOver( const QString&, int, int )) );
	connect( eventFilter, SIGNAL(wordUnhovered()), this, SLOT(slotWordUnhovered()) );

	internalView->installEventFilter( eventFilter );
#endif

    // TODO HACK disable some actions which collide with ktechlab's actions.
    //  the proper solution would be to move the actions from KTechLab object level to document level for
    //  all types of documents
    for (QAction *act: actionCollection()->actions()) {
        qDebug() << Q_FUNC_INFO << "act: " << act->text() << " shortcut " << act->shortcut() << ":" << act ;

        if ( ((act->objectName()) == QLatin1String("file_save"))
            || ((act->objectName()) == QLatin1String("file_save_as"))
            || ((act->objectName()) == QLatin1String("file_print"))
            || ((act->objectName()) == QLatin1String("edit_undo"))
            || ((act->objectName()) == QLatin1String("edit_redo"))
            || ((act->objectName()) == QLatin1String("edit_cut"))
            || ((act->objectName()) == QLatin1String("edit_copy"))
            || ((act->objectName()) == QLatin1String("edit_paste"))
        ) {
            act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            //act->setShortcutConfigurable(true);
            act->setShortcut(Qt::Key_unknown);
            qDebug() << Q_FUNC_INFO << "action " << act << " disabled";
        }
    }
}


TextView::~TextView()
{
	if ( KTechlab::self() ) {
        // 2017.01.09: do not crash on document close. factory has its clients removed in TextDocument::~TextDocument()
		//if ( KXMLGUIFactory * f = m_view->factory() )
		//	f->removeClient( m_view );
		KTechlab::self()->addNoRemoveGUIClient( m_view );
	}

	delete m_pViewIface;
}


bool TextView::closeView()
{
	if ( textDocument() )
	{
		const QString path = textDocument()->url().prettyUrl();
		if ( !path.isEmpty() )
			fileMetaInfo()->grabMetaInfo( path, this );
	}

	bool doClose = View::closeView();
	if ( doClose )
		KTechlab::self()->factory()->removeClient(m_view);
	return View::closeView();
}

bool TextView::gotoLine( const int line ) {
    //return m_view->setCursorPosition( line, 0/*m_view->cursorColumn()*/ );
    return m_view->setCursorPosition( KTextEditor::Cursor( line, 0/*m_view->cursorColumn()*/ ) );
}

TextDocument *TextView::textDocument() const
{
	return static_cast<TextDocument*>(document());
}
void TextView::undo() {
    qDebug() << Q_FUNC_INFO;
    // note: quite a hack, but could not find any more decent way of getting to undo/redo interface
    // note: quite a hack, but could not find any more decent way of getting to undo/redo interface
    QAction* action = actionByName("edit_undo");
    if (action) {
        action->trigger();
        return;
    }
    qWarning() << Q_FUNC_INFO << "no edit_undo action in text view! no action taken";
}
void TextView::redo() {
    qDebug() << Q_FUNC_INFO;
    // note: quite a hack, but could not find any more decent way of getting to undo/redo interface
    QAction* action = actionByName("edit_redo");
    if (action) {
        action->trigger();
        return;
    }
    qWarning() << Q_FUNC_INFO << "no edit_redo action in text view! no action taken";
}

void TextView::cut() {
    //m_view-> cut();
    if (!m_view->selection()) return;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( m_view->document()->text( m_view->selectionRange() ) );
    m_view->document()->removeText(m_view->selectionRange());
}

void TextView::copy() {
    //m_view->copy();
    if (!m_view->selection()) return;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( m_view->document()->text( m_view->selectionRange() ) );
}

void TextView::paste() {
    //m_view->paste();
    QClipboard *clipboard = QApplication::clipboard();
    m_view->document()->insertText( m_view->cursorPosition(), clipboard->text());
}


void TextView::disableActions()
{
	QMenu * tb = (dynamic_cast<KToolBarPopupAction*>(actionByName("program_convert")))->menu();

    QList<QAction*> actions = tb->actions();
    Q_FOREACH(QAction *a, actions) {
        switch (a->data().toInt()) {
            case TextDocument::AssemblyOutput:
            case TextDocument::HexOutput:
            case TextDocument::PICOutput:
                a->setEnabled( false );
                break;
            default:
                qDebug() << Q_FUNC_INFO << " skip action: " << a;
        }
    }
	//tb->setItemEnabled( TextDocument::AssemblyOutput, false );    // 2018.12.02
	//tb->setItemEnabled( TextDocument::HexOutput, false );
	//tb->setItemEnabled( TextDocument::PICOutput, false );
	actionByName("format_asm")->setEnabled(false);

#ifndef NO_GPSIM
	actionByName("debug_toggle_breakpoint")->setEnabled(false);
#endif
}

//KTextEditor::View::saveResult TextView::save() { return m_view->save(); }
bool TextView::save()
{
    return (m_view->document()->documentSave());
}

//KTextEditor::View::saveResult TextView::saveAs() { return m_view->saveAs(); }
bool TextView::saveAs()
{
    return m_view->document()->documentSaveAs();
}
void TextView::print() {
    qDebug() << Q_FUNC_INFO;
    // note: quite a hack, but could not find any more decent way of getting to undo/redo interface
    QAction* action = actionByName("file_print");
    if (action) {
        action->trigger();
        return;
    }
    qWarning() << Q_FUNC_INFO << "no file_print action in text view! no action taken";
}

void TextView::gotFocus()
{
#ifndef NO_GPSIM
	GpsimDebugger * debugger = textDocument()->debugger();
	if ( !debugger || !debugger->gpsim() )
		return;

	SymbolViewer::self()->setContext( debugger->gpsim() );
#endif
}

void TextView::slotSelectionmChanged() {
    KTechlab::self()->actionByName( "edit_cut" )->setEnabled( m_view->selection() );
    KTechlab::self()->actionByName( "edit_copy" )->setEnabled( m_view->selection() );
}

void TextView::initCodeActions()
{
	disableActions();

	QMenu * tb = (dynamic_cast<KToolBarPopupAction*>(actionByName("program_convert")))->menu();

    QAction *actHexOut = NULL;
    QAction *actPicOut = NULL;
    QAction *actAsmOut = NULL;
    QList<QAction*> actions = tb->actions();
    Q_FOREACH(QAction *a, actions) {
        switch (a->data().toInt()) {
            case TextDocument::AssemblyOutput:  actAsmOut = a; break;
            case TextDocument::HexOutput:       actHexOut = a; break;
            case TextDocument::PICOutput:       actPicOut = a; break;
            default:
                qDebug() << Q_FUNC_INFO << " skip action: " << a;
        }
    }

	switch ( textDocument()->guessedCodeType() )
	{
		case TextDocument::ct_asm:
		{
			//tb->setItemEnabled( TextDocument::HexOutput, true );  // 2018.12.02
			//tb->setItemEnabled( TextDocument::PICOutput, true );
            actHexOut->setEnabled( true );
            actPicOut->setEnabled( true );
			actionByName("format_asm")->setEnabled(true);
#ifndef NO_GPSIM
			actionByName("debug_toggle_breakpoint")->setEnabled(true);
			slotInitDebugActions();
#endif
			break;
		}
		case TextDocument::ct_c:
		{
			//tb->setItemEnabled( TextDocument::AssemblyOutput, true );
			//tb->setItemEnabled( TextDocument::HexOutput, true );
			//tb->setItemEnabled( TextDocument::PICOutput, true );
            actAsmOut->setEnabled( true );
            actHexOut->setEnabled( true );
            actPicOut->setEnabled( true );
			break;
		}
		case TextDocument::ct_hex:
		{
			//tb->setItemEnabled( TextDocument::AssemblyOutput, true );
			//tb->setItemEnabled( TextDocument::PICOutput, true );
            actAsmOut->setEnabled( true );
            actPicOut->setEnabled( true );
			break;
		}
		case TextDocument::ct_microbe:
		{
			//tb->setItemEnabled( TextDocument::AssemblyOutput, true );
			//tb->setItemEnabled( TextDocument::HexOutput, true );
			//tb->setItemEnabled( TextDocument::PICOutput, true );
            actAsmOut->setEnabled( true );
            actHexOut->setEnabled( true );
            actPicOut->setEnabled( true );
			break;
		}
		case TextDocument::ct_unknown:
		{
			break;
		}
	}
}

void TextView::setCursorPosition( uint line, uint col ) {
    //m_view->setCursorPosition( line, col );
    m_view->setCursorPosition( KTextEditor::Cursor( line, col ) );
}

unsigned TextView::currentLine()
{
	//unsigned l,c ;
	KTextEditor::Cursor curs = m_view->cursorPosition();
	return curs.line();
}
unsigned TextView::currentColumn()
{
	//unsigned l,c ;
	KTextEditor::Cursor curs = m_view->cursorPosition(); // &l, &c );
	return curs.column();
}


void TextView::saveCursorPosition()
{
	KTextEditor::Cursor curs = m_view->cursorPosition(); // &m_savedCursorLine, &m_savedCursorColumn );
	m_savedCursorLine = curs.line();
    m_savedCursorColumn = curs.column();
}


void TextView::restoreCursorPosition()
{
	m_view->setCursorPosition( KTextEditor::Cursor(  m_savedCursorLine, m_savedCursorColumn ) );
}


void TextView::slotCursorPositionChanged()
{
	uint line, column;
	KTextEditor::Cursor curs = m_view->cursorPosition(); //&line, &column );
	line = curs.line();
    column = curs.column();

	m_statusBar->changeItem( i18n(" Line: %1 Col: %2 ", QString::number(line+1), QString::number(column+1)), ViewStatusBar::LineCol );

	slotUpdateMarksInfo();
}


void TextView::slotUpdateMarksInfo()
{
#ifndef NO_GPSIM
	uint l;
	KTextEditor::Cursor curs = m_view->cursorPosition(); // &l, &c );
	l = curs.line();
    KTextEditor::MarkInterface *iface = qobject_cast<KTextEditor::MarkInterface*>( m_view->document() );
	// if ( m_view->getDoc()->mark(l) & TextDocument::Breakpoint )
    if (iface->mark(l) & TextDocument::Breakpoint)
		actionByName("debug_toggle_breakpoint")->setText( i18n("Clear &Breakpoint") );
	else
		actionByName("debug_toggle_breakpoint")->setText( i18n("Set &Breakpoint") );
#endif
}


void TextView::slotInitDebugActions()
{
#ifndef NO_GPSIM
	bool isRunning = textDocument()->debuggerIsRunning();
	bool isStepping = textDocument()->debuggerIsStepping();
	bool ownDebugger = textDocument()->ownDebugger();

	actionByName("debug_run")->setEnabled( !isRunning || isStepping );
	actionByName("debug_interrupt")->setEnabled(isRunning && !isStepping);
	actionByName("debug_stop")->setEnabled(isRunning && ownDebugger);
	actionByName("debug_step")->setEnabled(isRunning && isStepping);
	actionByName("debug_step_over")->setEnabled(isRunning && isStepping);
	actionByName("debug_step_out")->setEnabled(isRunning && isStepping);
#endif // !NO_GPSIM
}


void TextView::toggleBreakpoint()
{
#ifndef NO_GPSIM
	uint l;
	KTextEditor::Cursor curs = m_view->cursorPosition(); // &l, &c );
	l = curs.line();
    //const bool isBreakpoint = m_view->getDoc()->mark(l) & TextDocument::Breakpoint;
    KTextEditor::MarkInterface *iface = qobject_cast<KTextEditor::MarkInterface*>(m_view->document());
    if (!iface) return;
    const bool isBreakpoint = iface->mark(l) & TextDocument::Breakpoint;
	//textDocument()->setBreakpoint( l, !(m_view->getDoc()->mark(l) & TextDocument::Breakpoint) );
    textDocument()->setBreakpoint(l, !isBreakpoint);
#endif // !NO_GPSIM
}


void TextView::slotWordHoveredOver( const QString & word, int line, int /*col*/ )
{
#ifndef NO_GPSIM
	// We're only interested in popping something up if we currently have a debugger running
	GpsimProcessor * gpsim = textDocument()->debugger() ? textDocument()->debugger()->gpsim() : 0l;
	if ( !gpsim )
	{
		m_pTextViewLabel->hide();
		return;
	}

	// Find out if the word that we are hovering over is the operand data
	//KTextEditor::EditInterface * e = (KTextEditor::EditInterface*)textDocument()->kateDocument()->qt_cast("KTextEditor::EditInterface");
	//InstructionParts parts( e->textLine( unsigned(line) ) );
    InstructionParts parts(textDocument()->kateDocument()->line(line));
	if ( !parts.operandData().contains( word ) )
		return;

	if ( RegisterInfo * info = gpsim->registerMemory()->fromName( word ) )
		m_pTextViewLabel->setRegister( info, info->name() );

	else
	{
		int operandAddress = textDocument()->debugger()->programAddress( textDocument()->debugFile(), line );
		if ( operandAddress == -1 )
		{
			m_pTextViewLabel->hide();
			return;
		}

		int regAddress = gpsim->operandRegister( operandAddress );

		if ( regAddress != -1 )
			m_pTextViewLabel->setRegister( gpsim->registerMemory()->fromAddress( regAddress ), word );

		else
		{
			m_pTextViewLabel->hide();
			return;
		}
	}

	m_pTextViewLabel->move( mapFromGlobal( QCursor::pos() ) + QPoint( 0, 20 ) );
	m_pTextViewLabel->show();
#endif // !NO_GPSIM
}


void TextView::slotWordUnhovered()
{
#ifndef NO_GPSIM
	m_pTextViewLabel->hide();
#endif // !NO_GPSIM
}
//END class TextView



//BEGIN class TextViewEventFilter
TextViewEventFilter::TextViewEventFilter( TextView * textView )
{
	m_hoverStatus = Sleeping;
	m_pTextView = textView;
	m_lastLine = m_lastCol = -1;

	//((KTextEditor::TextHintInterface*)textView->kateView()->qt_cast("KTextEditor::TextHintInterface"))->enableTextHints(0);
    {
        KTextEditor::View * view = textView->kateView();
        KTextEditor::TextHintInterface *iface = qobject_cast<KTextEditor::TextHintInterface*>(view);
        if (iface) {
#warning "TextHintProvider"
            //iface->enableTextHints(0);
            //iface->registerTextHintProvider(); // TODO
            //connect( textView->kateView(), SIGNAL(needTextHint(int, int, QString &)), this, SLOT(slotNeedTextHint( int, int, QString& )) );
            connect( view, SIGNAL(needTextHint(const KTextEditor::Cursor &, QString &)),
                     this, SLOT(slotNeedTextHint(const KTextEditor::Cursor &, QString &)) );
        } else {
            qWarning() << "KTextEditor::View does not implement TextHintInterface for " << view;
        }
    }

	m_pHoverTimer = new QTimer( this );
	connect( m_pHoverTimer, SIGNAL(timeout()), this, SLOT(hoverTimeout()) );

	m_pSleepTimer = new QTimer( this );
	connect( m_pSleepTimer, SIGNAL(timeout()), this, SLOT(gotoSleep()) );

	m_pNoWordTimer = new QTimer( this );
	connect( m_pNoWordTimer, SIGNAL(timeout()), this, SLOT(slotNoWordTimeout()) );
}


bool TextViewEventFilter::eventFilter( QObject *, QEvent * e )
{
// 	qDebug() << Q_FUNC_INFO << "e->type() = " << e->type() << endl;

	if ( e->type() == QEvent::MouseMove )
	{
		if ( !m_pNoWordTimer->isActive() )
			m_pNoWordTimer->start( 10 );
		return false;
	}

	if ( e->type() == QEvent::FocusOut || e->type() == QEvent::FocusIn || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::Leave || e->type() == QEvent::Wheel )
	{
		// user moved focus somewhere - hide the tip and sleep
		if ( ((QFocusEvent*)e)->reason() != Qt::PopupFocusReason )
			updateHovering( 0, -1, -1 );
	}

	return false;
}


void TextViewEventFilter::hoverTimeout()
{
	m_pSleepTimer->stop();
	m_hoverStatus = Active;
	emit wordHoveredOver( m_lastWord, m_lastLine, m_lastCol );
}


void TextViewEventFilter::gotoSleep()
{
	m_hoverStatus = Sleeping;
	m_lastWord = QString::null;
	emit wordUnhovered();
	m_pHoverTimer->stop();
}


void TextViewEventFilter::slotNoWordTimeout()
{
	updateHovering( 0, -1, -1 );
}


void TextViewEventFilter::updateHovering( const QString & currentWord, int line, int col )
{
	if ( (currentWord == m_lastWord) && (line == m_lastLine) )
		return;

	m_lastWord = currentWord;
	m_lastLine = line;
	m_lastCol = col;

	if ( currentWord.isEmpty() )
	{
		if ( m_hoverStatus == Active )
			m_hoverStatus = Hidden;

		emit wordUnhovered();
		if ( !m_pSleepTimer->isActive() ) {
            m_pSleepTimer->setSingleShot( true );
			m_pSleepTimer->start( 2000 /*, true */ );
        }
		return;
	}

	if ( m_hoverStatus != Sleeping ) {
		emit wordHoveredOver( currentWord, line, col );
    } else {
        m_pHoverTimer->setSingleShot( true );
		m_pHoverTimer->start( 700 /*, true */ );
    }
}


static inline bool isWordLetter( const QString & s ) { return (s.length() == 1) && (s[0].isLetterOrNumber() || s[0] == '_'); }


void TextViewEventFilter::slotNeedTextHint(const KTextEditor::Cursor &position, QString &  /*text*/)
{
    int line = position.line();
    int col = position.column();
	m_pNoWordTimer->stop();

	//KTextEditor::EditInterface * e = (KTextEditor::EditInterface*)m_pTextView->textDocument()->kateDocument()->qt_cast("KTextEditor::EditInterface");
    KTextEditor::Document *d = m_pTextView->textDocument()->kateDocument();

	// Return if we aren't currently in a word
	if ( !isWordLetter( d->text( KTextEditor::Range( line, col, line, col+1 ) ) ) )
	{
		updateHovering( QString::null, line, col );
		return;
	}

	// Find the start of the word
	int wordStart = col;
	do wordStart--;
	while ( wordStart > 0 && isWordLetter( d->text( KTextEditor::Range( line, wordStart, line, wordStart+1 ) ) ) );
	wordStart++;

	// Find the end of the word
	int wordEnd = col;
	do wordEnd++;
	while ( isWordLetter( d->text( KTextEditor::Range( line, wordEnd, line, wordEnd+1 ) ) ) );

	QString t = d->text( KTextEditor::Range( line, wordStart, line, wordEnd ) );

	updateHovering( t, line, col );
}
//END class TextViewEventFilter



#include "moc_textview.cpp"
