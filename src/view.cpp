#include "document.h"
#include "iteminterface.h"
#include "ktechlab.h"
#include "view.h"
#include "viewiface.h"
#include "viewcontainer.h"

#include <QDebug>
#include <KIconLoader>
#include <KLocalizedString>
#include <KSqueezedTextLabel>
#include <KXMLGUIFactory>
#include <KActionCollection>

#include <QApplication>
#include <QBoxLayout>

#include <cassert>

//BEGIN class View
View::View(Document *document, ViewContainer *viewContainer, int viewAreaId, const char *name) :
	QWidget(viewContainer->viewArea(viewAreaId)),
	KXMLGUIClient(),
	m_pDocument(document),
	p_viewContainer(viewContainer),
	m_viewAreaId(viewAreaId)
{
  setObjectName(name ?: ("view_" + QString::number(viewAreaId)).toLatin1().data());

	setFocusPolicy(Qt::ClickFocus);

	if (ViewArea *viewArea = viewContainer->viewArea(viewAreaId)) {
		viewArea->setView(this);
	}
	else {
		qDebug() << Q_FUNC_INFO << " viewArea = " << viewArea <<endl;
	}

	m_layout = new QVBoxLayout(this);

	// Don't bother creating statusbar if no ktechlab as we are not a main ktechlab tab
	if (KTechlab::self()) {
		m_statusBar = new ViewStatusBar(this);

		m_layout->addWidget(new KVSSBSep(this));
		m_layout->addWidget(m_statusBar);

		connect(KTechlab::self(), SIGNAL(configurationChanged()), this, SLOT(slotUpdateConfiguration()));
	}
}

View::~View() {
	delete m_statusBar;
	delete m_layout;
}

QAction* View::actionByName(const QString &name) const {
	QAction *action = actionCollection()->action(name);
	if (!action) {
		qCritical() << Q_FUNC_INFO << "No such action: " << name << endl;
	}
	return action;
}

DCOPObject * View::dcopObject( ) const {
	return m_pViewIface;
}

bool View::closeView() {
	return p_viewContainer->closeViewArea(viewAreaId());
}

void View::setFocusWidget(QWidget *focusWidget) {
	assert(focusWidget);
	assert(!m_pFocusWidget);

	if (hasFocus()) {
		focusWidget->setFocus();
	}

	m_pFocusWidget = focusWidget;
	setFocusProxy(m_pFocusWidget);
	m_pFocusWidget->installEventFilter(this);
	m_pFocusWidget->setFocusPolicy(Qt::ClickFocus);
}

bool View::eventFilter(QObject *watched, QEvent *e) {
	if (watched != m_pFocusWidget) {
		return false;
	}

	switch (e->type()) {
		case QEvent::FocusIn: {
			p_viewContainer->setActiveViewArea(viewAreaId());

			if (KTechlab * ktl = KTechlab::self()) {
				ktl->actionByName("file_save")->setEnabled(true);
				ktl->actionByName("file_save_as")->setEnabled(true);
				ktl->actionByName("file_close")->setEnabled(true);
				ktl->actionByName("file_print")->setEnabled(true);
				ktl->actionByName("edit_paste")->setEnabled(true);
				ktl->actionByName("view_split_leftright")->setEnabled(true);
				ktl->actionByName("view_split_topbottom")->setEnabled(true);

				ItemInterface::self()->updateItemActions();
			}

			emit focused(this);
			break;
		}

		case QEvent::FocusOut: {
      QFocusEvent *fe = static_cast<QFocusEvent*>(e);
			if (QWidget * fw = qApp->focusWidget()) {
				QString fwClassName( fw->metaObject()->className() );

				if ((fwClassName != "KateViewInternal") && (fwClassName != "QViewportWidget")) {
					break;
				}
			}

			if (fe->reason() == Qt::PopupFocusReason) {
				break;
			}

			if (fe->reason() == Qt::ActiveWindowFocusReason) {
				break;
			}

			emit unfocused();
			break;
		}

		default:
			break;
	}

	return false;
}

void View::setDCOPID(signed id) {
	if (m_dcopID == id)
		return;

	m_dcopID = id;
	if (m_pViewIface) {
		QString docID;
		docID.setNum(document()->dcopID());

		QString viewID;
		viewID.setNum(dcopID());

		m_pViewIface->setObjId("View#" + docID + "." + viewID);
	}
}
//END class View

//BEGIN class ViewStatusBar
ViewStatusBar::ViewStatusBar(View *view) :
	KStatusBar(view),
	p_view(view)
{
	m_modifiedLabel = new QLabel(this);
	addWidget(m_modifiedLabel, 0);
	m_fileNameLabel = new KSqueezedTextLabel(this);
	addWidget(m_fileNameLabel, 1);

	m_modifiedPixmap = KIconLoader::global()->loadIcon("document-save", KIconLoader::Small);
	m_unmodifiedPixmap = KIconLoader::global()->loadIcon("null", KIconLoader::Small);

	connect(view->document(), SIGNAL(modifiedStateChanged()), this, SLOT(slotModifiedStateChanged()));
	connect(view->document(), SIGNAL(fileNameChanged(const KUrl &)), this, SLOT(slotFileNameChanged(const KUrl &)));

	connect(view, SIGNAL(focused(View *)), this, SLOT(slotViewFocused(View *)));
	connect(view, SIGNAL(unfocused()), this, SLOT(slotViewUnfocused()));

	slotModifiedStateChanged();
	slotFileNameChanged(view->document()->url());

	slotViewUnfocused();
}

void ViewStatusBar::slotModifiedStateChanged() {
	m_modifiedLabel->setPixmap(
		p_view->document()->isModified() ? m_modifiedPixmap : m_unmodifiedPixmap
	);
}

void ViewStatusBar::slotFileNameChanged(const KUrl &url) {
	m_fileNameLabel->setText(
		url.isEmpty() ? i18n("Untitled") : url.fileName(KUrl::IgnoreTrailingSlash)
	);
}

void ViewStatusBar::slotViewFocused(View *) {
	setPalette(p_view->palette());
}

void ViewStatusBar::slotViewUnfocused() {
	QPalette pal(p_view->palette());
	pal.setColor(QPalette::Window, pal.mid().color());
	pal.setColor(QPalette::Light, pal.midlight().color());
	setPalette(pal);
}
//END class ViewStatusBar

//BEGIN class KVSSBSep
void KVSSBSep::paintEvent(QPaintEvent *e) {
	QPainter p;
	const bool beginSuccess = p.begin(this);
	if (!beginSuccess) {
		qWarning() << Q_FUNC_INFO << " painter is not active";
	}

	p.setPen( palette().shadow().color() );
	p.drawLine(
		e->rect().left(), 0,
		e->rect().right(), 0
	);
	View *parentView = dynamic_cast<View*>(parentWidget());
	if (!parentView) {
		qWarning() << "parent not View for this=" << this << ", parent=" << parentWidget();
		return;
	}
	p.setPen(
		parentView->hasFocus() ? palette().light().color() : palette().midlight().color()
	);
	p.drawLine(
		e->rect().left(), 1,
		e->rect().right(), 1
	);
}
//END  class KVSSBSep

#include "moc_view.cpp"
