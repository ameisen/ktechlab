#define protected public
#include <KXMLGUIClient>
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

#include <ktexteditor/texthintinterface.h>

#include <KAction>
#include <KDebug>
#include <QDebug>
#include <KIconLoader>
#include <KIcon>
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardDirs>
#include <KToolBarPopupAction>
#include <KMenu>
#include <KXMLGUIFactory>
#include <KActionCollection>
#include <KAction>
#include <QActionGroup>

#include <QApplication>
#include <QBoxLayout>
#include <QCursor>
#include <QTimer>
#include <QSharedPointer>
#include <QClipboard>

namespace {
	struct DebugAction final {
		const char * const theme = "";
		const char * const text;
		const char * const name;
		const decltype(SLOT(toggleBreakpoint())) functor;
		enum class SlotObject {
			This,
			Document
		};
		const SlotObject object = SlotObject::Document;
		const int shortcut = 0;
	};
}

//BEGIN class TextView
TextView::TextView(TextDocument *textDocument, ViewContainer *viewContainer, int viewAreaId, const char *name) :
	View(textDocument, viewContainer, viewAreaId, name),
	m_view(textDocument->createKateView(this))
{
	m_view->insertChildClient(this);

  auto *actionList = actionCollection();

	//BEGIN Convert To * Actions
  auto *popupAction = new KToolBarPopupAction( QIcon::fromTheme("fork"), i18n("Convert To"), actionList);
  popupAction->setObjectName("program_convert");
	popupAction->setDelayed(false);
  actionList->addAction(popupAction->objectName(), popupAction);

	auto *popupMenu = popupAction->menu();

  popupMenu->setTitle( i18n("Convert To") );
  auto *actToMicrobe = popupMenu->addAction( QIcon::fromTheme("convert_to_microbe"), i18n("Microbe"));
  actToMicrobe->setData(TextDocument::MicrobeOutput);
	popupMenu->addAction( QIcon::fromTheme("convert_to_assembly"), i18n("Assembly"))->setData(TextDocument::AssemblyOutput);
	popupMenu->addAction( QIcon::fromTheme("convert_to_hex"), i18n("Hex"))->setData(TextDocument::HexOutput);
	popupMenu->addAction( QIcon::fromTheme("convert_to_pic"), i18n("PIC (upload)"))->setData(TextDocument::PICOutput);
	connect(
		popupMenu, SIGNAL(triggered(QAction *)),
		textDocument, SLOT(slotConvertTo(QAction *))
	);

  actToMicrobe->setEnabled(false);
  actionList->addAction(popupAction->objectName(), popupAction);
	//END Convert To * Actions

	static const DebugAction actions[] = {
		{
			.text = "Format Assembly Code",
			.name = "format_asm",
			.functor = SLOT(formatAssembly()),
		},
#ifndef NO_GPSIM
		{
			.text = "Set &Breakpoint",
			.name = "debug_toggle_breakpoint",
			.functor = SLOT(toggleBreakpoint()),
			.object = DebugAction::SlotObject::This
		},
		{
			.theme = "debug-run",
			.text = "Run",
			.name = "debug_run",
			.functor = SLOT(debugRun())
		},
		{
			.theme = "media-playback-pause",
			.text = "Interrupt",
			.name = "debug_interrupt",
			.functor = SLOT(debugInterrupt())
		},
		{
			.theme = "process-stop",
			.text = "Stop",
			.name = "debug_stop",
			.functor = SLOT(debugStop())
		},
		{
			.theme = "debug-step-instruction",
			.text = "Step",
			.name = "debug_step",
			.functor = SLOT(debugStep()),
			.shortcut = (Qt::CTRL | Qt::ALT | Qt::Key_Right)
		},
		{
			.theme = "debug-step-over",
			.text = "Step Over",
			.name = "debug_step_over",
			.functor = SLOT(debugStepOver())
		},
		{
			.theme = "debug-step-out",
			.text = "Step Out",
			.name = "debug_step_out",
			.functor = SLOT(debugStepOut())
		}
	};
#endif

	for (const auto &debugAction : actions) {
		auto *action = new QAction(
			QIcon::fromTheme(debugAction.theme),
			i18n(debugAction.text),
			actionList
		);
		action->setObjectName(debugAction.name);

		if (debugAction.shortcut != 0) {
			action->setShortcut(debugAction.shortcut);
		}

		using QObjectPtr = QObject *;
		QObjectPtr slotObject = (debugAction.object == DebugAction::SlotObject::This) ? QObjectPtr(this) : QObjectPtr(textDocument);
		connect(
			action, SIGNAL(triggered(bool)),
			slotObject, debugAction.functor
		);

		actionList->addAction(action->objectName(), action);
	}

	static const QString resourceFile = "ktechlabtextui.rc";

	setXMLFile(resourceFile);
	m_view->setXMLFile(KStandardDirs::locate("appdata", resourceFile));

	m_pViewIface = new TextViewIface(this);

	setAcceptDrops(true);

	m_statusBar->insertItem("", int(ViewStatusBar::InfoId::LineCol));

  m_view->setContextMenu(static_cast<QMenu*>(KTechlab::self()->factory()->container("ktexteditor_popup", KTechlab::self())));

	auto *internalView = static_cast<QWidget*>(ktlFindQObjectChild(m_view, "KateViewInternal"));

	connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor &)),this, SLOT(slotCursorPositionChanged()));
	connect(m_view, SIGNAL(selectionChanged(KTextEditor::View *)), this, SLOT(slotSelectionmChanged()));

	setFocusWidget(internalView);
	connect(this, SIGNAL(focused(View*)), this, SLOT(gotFocus()));

	m_layout->insertWidget(0, m_view);

	slotCursorPositionChanged();
	slotInitDebugActions();
	initCodeActions();

#ifndef NO_GPSIM
	m_pTextViewLabel = new VariableLabel(this);
	m_pTextViewLabel->hide();

	auto *eventFilter = new TextViewEventFilter(this);
	connect(eventFilter, SIGNAL(wordHoveredOver(const QString &, int, int)), this, SLOT(slotWordHoveredOver(const QString &, int, int)));
	connect(eventFilter, SIGNAL(wordUnhovered()), this, SLOT(slotWordUnhovered()));

	internalView->installEventFilter(eventFilter);
#endif

	// TODO HACK disable some actions which collide with ktechlab's actions.
	//  the proper solution would be to move the actions from KTechLab object level to document level for
	//  all types of documents
	for (auto *act : actionCollection()->actions()) {
		qDebug() << Q_FUNC_INFO << "act: " << act->text() << " shortcut " << act->shortcut() << ":" << act ;

		auto objectName = act->objectName();

		bool matches = false;
		for (const char *comparand : {
				"file_save",
				"file_save_as",
				"file_print",
				"edit_undo",
				"edit_redo",
				"edit_cut",
				"edit_copy",
				"edit_paste"
		}) {
			if ((matches = (objectName == QLatin1String(comparand)))) {
				break;
			}
		}

		if (matches) {
			act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
			act->setShortcut(Qt::Key_unknown);
			qDebug() << Q_FUNC_INFO << "action " << act << " disabled";
		}
	}
}

TextView::~TextView() {
	if (KTechlab::self()) {
		KTechlab::self()->addNoRemoveGUIClient(m_view);
	}

	delete m_pViewIface;
}

bool TextView::closeView() {
	if (textDocument()) {
		const auto path = textDocument()->url().prettyUrl();
		if (!path.isEmpty()) {
			fileMetaInfo()->grabMetaInfo(path, this);
		}
	}

	if (View::closeView()) {
		KTechlab::self()->factory()->removeClient(m_view);
	}
	return View::closeView();
}

bool TextView::gotoLine(const int line) {
	return m_view->setCursorPosition(KTextEditor::Cursor(line, 0));
}

TextDocument *TextView::textDocument() const {
	return static_cast<TextDocument *>(document());
}

void TextView::undo() {
    qDebug() << Q_FUNC_INFO;

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
    if (!m_view->selection()) return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_view->document()->text(m_view->selectionRange()));
    m_view->document()->removeText(m_view->selectionRange());
}

void TextView::copy() {
    if (!m_view->selection()) return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_view->document()->text(m_view->selectionRange()));
}

void TextView::paste() {
    QClipboard *clipboard = QApplication::clipboard();
    m_view->document()->insertText(m_view->cursorPosition(), clipboard->text());
}

void TextView::disableActions() {
	QMenu *tb = (dynamic_cast<KToolBarPopupAction *>(actionByName("program_convert")))->menu();

  QList<QAction*> actions = tb->actions();
	for (auto &a : actions) {
		switch (a->data().toInt()) {
			case TextDocument::AssemblyOutput:
			case TextDocument::HexOutput:
			case TextDocument::PICOutput:
				a->setEnabled(false);
				break;
			default:
				qDebug() << Q_FUNC_INFO << " skip action: " << a;
		}
	}

	actionByName("format_asm")->setEnabled(false);

#ifndef NO_GPSIM
	actionByName("debug_toggle_breakpoint")->setEnabled(false);
#endif
}

bool TextView::save() {
  return m_view->document()->documentSave();
}

bool TextView::saveAs() {
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

void TextView::gotFocus() {
#ifndef NO_GPSIM
	auto *debugger = textDocument()->debugger();
	if (!debugger || !debugger->gpsim())
		return;

	SymbolViewer::self()->setContext(debugger->gpsim());
#endif
}

void TextView::slotSelectionmChanged() {
	KTechlab::self()->actionByName("edit_cut")->setEnabled(m_view->selection());
	KTechlab::self()->actionByName("edit_copy")->setEnabled(m_view->selection());
}

void TextView::initCodeActions() {
	disableActions();

	QMenu *tb = (dynamic_cast<KToolBarPopupAction *>(actionByName("program_convert")))->menu();

QAction *actHexOut = nullptr;
QAction *actPicOut = nullptr;
QAction *actAsmOut = nullptr;
QList<QAction*> actions = tb->actions();
for (auto *a : actions) {
	switch (a->data().toInt()) {
		case TextDocument::AssemblyOutput:  actAsmOut = a; break;
		case TextDocument::HexOutput:       actHexOut = a; break;
		case TextDocument::PICOutput:       actPicOut = a; break;
		default:
			qDebug() << Q_FUNC_INFO << " skip action: " << a;
	}
}

switch (textDocument()->guessedCodeType()) {
	case TextDocument::ct_asm: {
		actHexOut->setEnabled(true);
		actPicOut->setEnabled(true);
		actionByName("format_asm")->setEnabled(true);
#ifndef NO_GPSIM
		actionByName("debug_toggle_breakpoint")->setEnabled(true);
		slotInitDebugActions();
#endif
		break;
	}
	case TextDocument::ct_c: {
		actAsmOut->setEnabled(true);
		actHexOut->setEnabled(true);
		actPicOut->setEnabled(true);
		break;
	}
	case TextDocument::ct_hex: {
		actAsmOut->setEnabled(true);
		actPicOut->setEnabled(true);
		break;
	}
	case TextDocument::ct_microbe: {
		actAsmOut->setEnabled(true);
		actHexOut->setEnabled(true);
		actPicOut->setEnabled(true);
		break;
	}
	case TextDocument::ct_unknown:
		break;
	}
}

void TextView::setCursorPosition(int line, int col) {
	m_view->setCursorPosition(KTextEditor::Cursor(line, col));
}

int TextView::currentLine() {
	KTextEditor::Cursor curs = m_view->cursorPosition();
	return curs.line();
}

int TextView::currentColumn() {
	KTextEditor::Cursor curs = m_view->cursorPosition();
	return curs.column();
}


void TextView::saveCursorPosition() {
	KTextEditor::Cursor curs = m_view->cursorPosition();
	m_savedCursorLine = curs.line();
	m_savedCursorColumn = curs.column();
}


void TextView::restoreCursorPosition() {
	m_view->setCursorPosition(KTextEditor::Cursor(m_savedCursorLine, m_savedCursorColumn));
}

void TextView::slotCursorPositionChanged() {
	KTextEditor::Cursor curs = m_view->cursorPosition();
	int line = curs.line();
	int column = curs.column();

	m_statusBar->changeItem(
		i18n(" Line: %1 Col: %2 ", QString::number(line + 1), QString::number(column + 1)), int(ViewStatusBar::InfoId::LineCol)
	);

	slotUpdateMarksInfo();
}

void TextView::slotUpdateMarksInfo() {
#ifndef NO_GPSIM
	KTextEditor::Cursor curs = m_view->cursorPosition();
	int l = curs.line();
	auto *iface = qobject_cast<KTextEditor::MarkInterface *>(m_view->document());

	if (iface->mark(l) & TextDocument::Breakpoint) {
		actionByName("debug_toggle_breakpoint")->setText(i18n("Clear &Breakpoint"));
	}
	else {
		actionByName("debug_toggle_breakpoint")->setText(i18n("Set &Breakpoint"));
	}
#endif
}

void TextView::slotInitDebugActions() {
#ifndef NO_GPSIM
	bool isRunning = textDocument()->debuggerIsRunning();
	bool isStepping = textDocument()->debuggerIsStepping();
	bool ownDebugger = textDocument()->ownDebugger();

	actionByName("debug_run")->setEnabled(!isRunning || isStepping);
	actionByName("debug_interrupt")->setEnabled(isRunning && !isStepping);
	actionByName("debug_stop")->setEnabled(isRunning && ownDebugger);
	actionByName("debug_step")->setEnabled(isRunning && isStepping);
	actionByName("debug_step_over")->setEnabled(isRunning && isStepping);
	actionByName("debug_step_out")->setEnabled(isRunning && isStepping);
#endif // !NO_GPSIM
}

void TextView::toggleBreakpoint() {
#ifndef NO_GPSIM
	KTextEditor::Cursor curs = m_view->cursorPosition();
	int l = curs.line();

	auto *iface = qobject_cast<KTextEditor::MarkInterface *>(m_view->document());
	if (!iface) return;

	const bool isBreakpoint = (iface->mark(l) & TextDocument::Breakpoint);
	textDocument()->setBreakpoint(l, !isBreakpoint);
#endif // !NO_GPSIM
}

void TextView::slotWordHoveredOver(const QString &word, int line, [[maybe_unused]] int col) {
#ifndef NO_GPSIM
	// We're only interested in popping something up if we currently have a debugger running
	GpsimProcessor *gpsim = textDocument()->debugger() ? textDocument()->debugger()->gpsim() : nullptr;
	if (!gpsim) {
		m_pTextViewLabel->hide();
		return;
	}

	// Find out if the word that we are hovering over is the operand data
	InstructionParts parts(textDocument()->kateDocument()->line(line));
	if (!parts.operandData().contains(word))
		return;

	if (RegisterInfo *info = gpsim->registerMemory()->fromName(word)) {
		m_pTextViewLabel->setRegister(info, info->name());
	}
	else {
		int operandAddress = textDocument()->debugger()->programAddress(textDocument()->debugFile(), line);
		if (operandAddress == -1) {
			m_pTextViewLabel->hide();
			return;
		}

		int regAddress = gpsim->operandRegister(operandAddress);

		if (regAddress != -1) {
			m_pTextViewLabel->setRegister(gpsim->registerMemory()->fromAddress(regAddress), word);
		}
		else {
			m_pTextViewLabel->hide();
			return;
		}
	}

	m_pTextViewLabel->move(mapFromGlobal(QCursor::pos()) + QPoint(0, 20));
	m_pTextViewLabel->show();
#endif // !NO_GPSIM
}

void TextView::slotWordUnhovered() {
#ifndef NO_GPSIM
	m_pTextViewLabel->hide();
#endif // !NO_GPSIM
}
//END class TextView

//BEGIN class TextViewEventFilter
TextViewEventFilter::TextViewEventFilter(TextView *textView) :
	m_pHoverTimer(new QTimer(this)),
	m_pSleepTimer(new QTimer(this)),
	m_pNoWordTimer(new QTimer(this)),
	m_pTextView(textView)
{
	{
		KTextEditor::View *view = textView->kateView();
		KTextEditor::TextHintInterface *iface = qobject_cast<KTextEditor::TextHintInterface *>(view);
		if (iface) {
#warning "TextHintProvider"
			//iface->enableTextHints(0);
			//iface->registerTextHintProvider(); // TODO
			//connect( textView->kateView(), SIGNAL(needTextHint(int, int, QString &)), this, SLOT(slotNeedTextHint( int, int, QString& )) );
			connect(
				view, SIGNAL(needTextHint(const KTextEditor::Cursor &, QString &)),
				this, SLOT(slotNeedTextHint(const KTextEditor::Cursor &, QString &))
			);
		}
		else {
			qWarning() << "KTextEditor::View does not implement TextHintInterface for " << view;
		}
	}

	connect(m_pHoverTimer, SIGNAL(timeout()), this, SLOT(hoverTimeout()));
	connect(m_pSleepTimer, SIGNAL(timeout()), this, SLOT(gotoSleep()));
	connect(m_pNoWordTimer, SIGNAL(timeout()), this, SLOT(slotNoWordTimeout()));
}

bool TextViewEventFilter::eventFilter(QObject *, QEvent * e) {
	if (e->type() == QEvent::MouseMove) {
		if (!m_pNoWordTimer->isActive()) {
			m_pNoWordTimer->start(10);
		}
		return false;
	}

	switch (e->type()) {
		case QEvent::FocusOut:
		case QEvent::FocusIn:
		case QEvent::MouseButtonPress:
		case QEvent::Leave:
		case QEvent::Wheel:
			// user moved focus somewhere - hide the tip and sleep
			if (static_cast<QFocusEvent *>(e)->reason() != Qt::PopupFocusReason) {
				updateHovering(0, -1, -1);
			}
			break;

		default:
			break;
	}

	return false;
}

void TextViewEventFilter::hoverTimeout() {
	m_pSleepTimer->stop();
	m_hoverStatus = HoverState::Active;
	emit wordHoveredOver(m_lastWord, m_lastLine, m_lastCol);
}

void TextViewEventFilter::gotoSleep() {
	m_hoverStatus = HoverState::Sleeping;
	m_lastWord = QString::null;
	emit wordUnhovered();
	m_pHoverTimer->stop();
}

void TextViewEventFilter::slotNoWordTimeout() {
	updateHovering(0, -1, -1);
}

void TextViewEventFilter::updateHovering(const QString &currentWord, int line, int col) {
	if ((currentWord == m_lastWord) && (line == m_lastLine))
		return;

	m_lastWord = currentWord;
	m_lastLine = line;
	m_lastCol = col;

	if (currentWord.isEmpty()) {
		if (m_hoverStatus == HoverState::Active) {
			m_hoverStatus = HoverState::Hidden;
		}

		emit wordUnhovered();
		if (!m_pSleepTimer->isActive()) {
    	m_pSleepTimer->setSingleShot(true);
			m_pSleepTimer->start(2000);
    }
		return;
	}

	if (m_hoverStatus != HoverState::Sleeping) {
		emit wordHoveredOver( currentWord, line, col );
  }
	else {
    m_pHoverTimer->setSingleShot(true);
		m_pHoverTimer->start(700);
  }
}

static inline bool isWordLetter(const QString &s) { return (s.length() == 1) && (s[0].isLetterOrNumber() || s[0] == '_'); }

void TextViewEventFilter::slotNeedTextHint(const KTextEditor::Cursor &position, [[maybe_unused]] const QString &text) {
	int line = position.line();
	int col = position.column();
	m_pNoWordTimer->stop();

	KTextEditor::Document *d = m_pTextView->textDocument()->kateDocument();

	// Return if we aren't currently in a word
	if (!isWordLetter(d->text(KTextEditor::Range(line, col, line, col + 1)))) {
		updateHovering(QString::null, line, col);
		return;
	}

	// Find the start of the word
	int wordStart = col;
	do {
		--wordStart;
	}
	while (wordStart > 0 && isWordLetter(d->text(KTextEditor::Range(line, wordStart, line, wordStart + 1))));
	++wordStart;

	// Find the end of the word
	int wordEnd = col;
	do {
		++wordEnd;
	}
	while (isWordLetter(d->text(KTextEditor::Range(line, wordEnd, line, wordEnd + 1))));

	QString t = d->text(KTextEditor::Range(line, wordStart, line, wordEnd));

	updateHovering(t, line, col);
}
//END class TextViewEventFilter

#include "moc_textview.cpp"
