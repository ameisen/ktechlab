#include "docmanager.h"
#include "document.h"
#include "itemview.h"
#include "ktechlab.h"
#include "view.h"
#include "viewcontainer.h"

#include <KConfig>
#include <QDebug>
#include <QBoxLayout>
#include <KGlobalSettings>
#include <KLocalizedString>
#include <KPushButton>
#include <KTabWidget>
#include <KConfigGroup>

//BEGIN class ViewContainer
ViewContainer::ViewContainer(const QString &caption, QWidget *parent) :
	QWidget(parent ?: KTechlab::self()->tabWidget())
{
	connect( KTechlab::self(), SIGNAL(needUpdateCaptions()), this, SLOT(updateCaption()) );

	// I assume these must eventually be deleted?
	QHBoxLayout *layout = new QHBoxLayout(this);
	m_baseViewArea = new ViewArea(this, this, 0, false, "viewarea_0");
	connect(m_baseViewArea, SIGNAL(destroyed(QObject*)), this, SLOT(baseViewAreaDestroyed(QObject*)));

	layout->addWidget(m_baseViewArea);

	setFocusProxy(m_baseViewArea);

	if (!parent) {
		KTechlab::self()->tabWidget()->addTab(this, caption);
		KTabWidget *tabWidget = KTechlab::self()->tabWidget();
		tabWidget->setCurrentIndex(tabWidget->indexOf(this));
	}

	show();
}

ViewContainer::~ViewContainer()
{
}

void ViewContainer::setActiveViewArea(int id) {
	if (m_activeViewArea == id)
		return;

	m_activeViewArea = id;
	View *newView = view(id);
	setFocusProxy(newView);

	if (newView) {
		setWindowTitle(newView->windowTitle());

		if (!DocManager::self()->getFocusedView() && newView->isVisible()) {
			newView->setFocus();
		}
	}
}

View *ViewContainer::view(int id) const {
	ViewArea *va = viewArea(id);
	if (!va)
		return nullptr;

	// We do not want a recursive search as ViewAreas also hold other ViewAreas
  auto l = va->findChildren<View*>();
	View *view = nullptr;
	if (!l.isEmpty()) {
		view = dynamic_cast<View*>(l.first());
	}

	return view;
}

ViewArea *ViewContainer::viewArea(int id) const {
	return m_viewAreaMap.value(id, nullptr);
}

bool ViewContainer::closeViewContainer() {
	bool didClose = true;
	while (didClose && !m_viewAreaMap.isEmpty()) {
		didClose = closeViewArea(m_viewAreaMap.begin().key());
	}

	return m_viewAreaMap.isEmpty();
}

bool ViewContainer::closeViewArea(int id) {
	ViewArea *va = viewArea(id);
	if (!va)
		return true;

	bool doClose = true;
	View *v = view(id);
	if (v && v->document()) {
		doClose = v->document()->numberOfViews() > 1;
		if (!doClose) {
			doClose = v->document()->fileClose();
		}
	}

	if (!doClose) {
		return false;
	}

	m_viewAreaMap.remove(id);
	va->deleteLater();

	if (m_activeViewArea == id){
		m_activeViewArea = -1;
		findActiveViewArea();
	}

	return true;
}

int ViewContainer::createViewArea(int relativeViewArea, ViewArea::Position position, bool showOpenButton) {
	if (relativeViewArea == -1)
		relativeViewArea = activeViewArea();

	ViewArea *relative = viewArea(relativeViewArea);
	if (!relative) {
		qCritical() << Q_FUNC_INFO << "Could not find relative view area" << endl;
		return -1;
	}

	int id = uniqueNewId();

	ViewArea *viewArea = relative->createViewArea(position, id, showOpenButton);
	viewArea->show(); // remove?

	return id;
}

void ViewContainer::setViewAreaId(ViewArea *viewArea, int id) {
	m_viewAreaMap[id] = viewArea;
	m_usedIDs.append(id);
}

void ViewContainer::setViewAreaRemoved(int id) {
	if (b_deleted)
		return;

	if (!m_viewAreaMap.remove(id)) {
		return;
	}

	if (m_activeViewArea == id) {
		findActiveViewArea();
	}
}

void ViewContainer::findActiveViewArea() {
	if (m_viewAreaMap.isEmpty())
		return;

	setActiveViewArea(m_viewAreaMap.lastKey());
}

void ViewContainer::baseViewAreaDestroyed(QObject *obj) {
	if (!obj)
		return;

	if (!b_deleted) {
		b_deleted = true;
		close();
		deleteLater();
	}
}

bool ViewContainer::canSaveUsefulStateInfo() const {
	return m_baseViewArea && m_baseViewArea->canSaveUsefulStateInfo();
}

void ViewContainer::saveState(KConfigGroup *config) {
	if (!m_baseViewArea)
		return;

	config->writeEntry("BaseViewArea", m_baseViewArea->id());
	m_baseViewArea->saveState(config);
}

void ViewContainer::restoreState(KConfigGroup *config, const QString &groupName) {
	int baseAreaId = config->readEntry("BaseViewArea", 0);
	m_baseViewArea->restoreState(config, baseAreaId, groupName);
}

int ViewContainer::uniqueParentId() {
	int lowest = -1;
	for (auto id : m_usedIDs) {
		lowest = std::min(lowest, id);
	}
	int newId = lowest - 1;
	m_usedIDs.append(newId);
	return newId;
}

int ViewContainer::uniqueNewId() {
	int highest = 0;
	for (auto id : m_usedIDs) {
		highest = std::max(highest, id);
	}
	int newId = highest + 1;
	m_usedIDs.append(newId);
	return newId;
}

void ViewContainer::setIdUsed(int id) {
	m_usedIDs.append(id);
}


void ViewContainer::updateCaption() {
	QString caption;

	if (!activeView() || !activeView()->document()) {
		caption = i18n("(empty)");
	}
	else {
		Document *doc = activeView()->document();
		caption = doc->url().isEmpty() ? doc->caption() : doc->url().fileName();
		if (viewCount() > 1) {
			caption += " ...";
		}
	}

	setWindowTitle(caption);

  KTechlab::self()->tabWidget()->setTabText(
  	KTechlab::self()->tabWidget()->indexOf(this),
		caption
	);
}
//END class ViewContainer

//BEGIN class ViewArea
ViewArea::ViewArea(QWidget *parent, ViewContainer *viewContainer, int id, bool showOpenButton, const char *name) :
	QSplitter(parent),
	p_viewContainer(viewContainer),
	m_id(id)
{
  setObjectName(name);

	if (id >= 0) {
		p_viewContainer->setViewAreaId(this, id);
	}

	p_viewContainer->setIdUsed(id);
	setOpaqueResize(KGlobalSettings::opaqueResize());

	if (showOpenButton) {
		m_pEmptyViewArea = new EmptyViewArea(this);
	}
}

ViewArea::~ViewArea() {
	if (m_id >= 0) {
		p_viewContainer->setViewAreaRemoved(m_id);
	}
}

ViewArea *ViewArea::createViewArea(Position position, int id, bool showOpenButton) {
	if (p_viewArea[0] || p_viewArea[1]) {
		qCritical() << Q_FUNC_INFO << "Attempting to create ViewArea when already containing ViewAreas!" << endl;
		return nullptr;
	}
	if (!p_view) {
		qCritical() << Q_FUNC_INFO << "We don't have a view yet, so creating a new ViewArea is redundant" << endl;
		return nullptr;
	}

	setOrientation((position == Position::Right) ? Qt::Horizontal : Qt::Vertical);

	p_viewArea[0] = new ViewArea(
		this,
		p_viewContainer,
		m_id,
		false,
    ("viewarea_" + QString::number(m_id)).toLatin1().data()
	);
	p_viewArea[1] = new ViewArea(
		this,
		p_viewContainer,
		id,
		showOpenButton,
    ("viewarea_" + QString::number(id)).toLatin1().data()
	);

	for (auto *viewArea : p_viewArea) {
		connect(viewArea, SIGNAL(destroyed(QObject*)), this, SLOT(viewAreaDestroyed(QObject*)));
	}

	p_view->clearFocus();
	p_view->setParent(p_viewArea[0]);
  p_view->move(QPoint());
  p_view->show();
	p_viewArea[0]->setView(p_view);
	setView(nullptr);

	m_id = p_viewContainer->uniqueParentId();

	// TODO : switch to QVector
	int pos = (orientation() == Qt::Horizontal) ? (width() / 2) : (height() / 2);
	QList<int> splitPos = { pos, pos };
	setSizes(splitPos);

	for (auto *viewArea : p_viewArea) {
		viewArea->show();
	}

	return p_viewArea[1];
}

void ViewArea::viewAreaDestroyed(QObject *obj) {
	auto *viewArea = static_cast<ViewArea*>(obj);

	for (auto *&pViewArea : p_viewArea) {
		if (pViewArea == viewArea) {
			pViewArea = nullptr;
		}
	}

	if (!p_viewArea[0] && !p_viewArea[1]) {
		deleteLater();
	}
}

void ViewArea::setView(View *view) {
	if (!view) {
		p_view = nullptr;
		setFocusProxy(nullptr);
		return;
	}

	delete m_pEmptyViewArea;
	m_pEmptyViewArea = nullptr;

	if (p_view) {
		qCritical() << Q_FUNC_INFO << "Attempting to set already contained view!" << endl;
		return;
	}

	p_view = view;

	connect(view, SIGNAL(destroyed()), this, SLOT(viewDestroyed()));
	bool hadFocus = hasFocus();
	setFocusProxy(p_view);
	if (hadFocus && !p_view->isHidden()) {
		p_view->setFocus();
	}

	// The ViewContainer by default has a view area as its focus proxy.
	// This is because there is no view when it is constructed. So give
	// it our view as the focus proxy if it doesn't have one.
	if (!dynamic_cast<View*>(p_viewContainer->focusProxy())) {
		p_viewContainer->setFocusProxy(p_view);
	}
}

void ViewArea::viewDestroyed() {
	if (!p_view && !p_viewArea[0] && !p_viewArea[1]) {
		deleteLater();
	}
}

bool ViewArea::canSaveUsefulStateInfo() const {
	for (auto *viewArea : p_viewArea) {
		if (viewArea && viewArea->canSaveUsefulStateInfo()) {
			return true;
		}
	}

	if (p_view && p_view->document() && !p_view->document()->url().isEmpty()) {
		return true;
	}

	return false;
}

void ViewArea::saveState(KConfigGroup *config) {
	bool va1Ok = p_viewArea[0] && p_viewArea[0]->canSaveUsefulStateInfo();
	bool va2Ok = p_viewArea[1] && p_viewArea[1]->canSaveUsefulStateInfo();

	if (va1Ok || va2Ok) {
		config->writeEntry(orientationKey(m_id), (orientation() == Qt::Horizontal) ? "LeftRight" : "TopBottom");

		QList<int> contains;
		contains.reserve(2);
		if (va1Ok) {
			contains << p_viewArea[0]->id();
		}
		if (va2Ok) {
			contains << p_viewArea[1]->id();
		}

		config->writeEntry(containsKey(m_id), contains);
		if (va1Ok) {
			p_viewArea[0]->saveState(config);
		}
		if (va2Ok) {
			p_viewArea[1]->saveState(config);
		}
	}
	else if (p_view && !p_view->document()->url().isEmpty()) {
		config->writePathEntry(fileKey(m_id), p_view->document()->url().prettyUrl());
	}
}

void ViewArea::restoreState(KConfigGroup *config, int id, const QString &groupName) {
	if (!config)
		return;

	if (id != m_id) {
		if (m_id >= 0) {
			p_viewContainer->setViewAreaRemoved(m_id);
		}

		m_id = id;

		if (m_id >= 0) {
			p_viewContainer->setViewAreaId(this, m_id);
		}

		p_viewContainer->setIdUsed(id);
	}
;
	if (config->hasKey(orientationKey(id))) {
		QString orientation = config->readEntry(orientationKey(m_id));
		setOrientation((orientation == "LeftRight") ? Qt::Horizontal : Qt::Vertical);
	}

	if (config->hasKey(containsKey(m_id))) {
		IntList contains = config->readEntry(containsKey(m_id), IntList());

		if (contains.isEmpty() || contains.size() > 2) {
			qCritical() << Q_FUNC_INFO << "Contained list has wrong size of " << contains.size() << endl;
		}
		else {
			if (contains.size() >= 1) {
				int viewArea1Id = contains[0];
				p_viewArea[0] = new ViewArea(
					this,
					p_viewContainer,
					viewArea1Id,
					false,
          ("viewarea_" + QString::number(viewArea1Id)).toLatin1().data()
				);
				connect(p_viewArea[0], SIGNAL(destroyed(QObject *)), this, SLOT(viewAreaDestroyed(QObject *)));
				p_viewArea[0]->restoreState(config, viewArea1Id, groupName);
				p_viewArea[0]->show();
			}
			if (contains.size() >= 2) {
				int viewArea2Id = contains[1];
				p_viewArea[1] = new ViewArea(
					this,
					p_viewContainer,
					viewArea2Id,
					false,
          ("viewarea_" + QString::number(viewArea2Id)).toLatin1().data()
				);
				connect(p_viewArea[1], SIGNAL(destroyed(QObject *)), this, SLOT(viewAreaDestroyed(QObject *)));
				p_viewArea[1]->restoreState(config, viewArea2Id, groupName);
				p_viewArea[1]->show();
			}
		}
	}

	if (config->hasKey(fileKey(m_id))) {
		bool openedOk = DocManager::self()->openURL(config->readPathEntry(fileKey(m_id), ""), this);
		if (!openedOk) {
			deleteLater();
		}
	}
}

QString ViewArea::fileKey(int id) {
	return QString("ViewArea ") + QString::number(id) + QString(" file");
}

QString ViewArea::containsKey(int id) {
	return QString("ViewArea ") + QString::number(id) + QString(" contains");
}

QString ViewArea::orientationKey(int id) {
	return QString("ViewArea ") + QString::number(id) + QString(" orientation");
}
//END class ViewArea

//BEGIN class EmptyViewArea
EmptyViewArea::EmptyViewArea(ViewArea *parent) :
	QWidget(parent),
	m_pViewArea(parent)
{
	QGridLayout *layout = new QGridLayout(this);
  layout->setMargin(0);
  layout->setSpacing(6);

	layout->setRowStretch(0, 20);
	layout->setRowStretch(2, 1);
	layout->setRowStretch(4, 20);

	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(2, 1);

	KGuiItem openItem(i18n("Open Document"), "document-open");
	KPushButton *newDocButton = new KPushButton(openItem, this);
	layout->addWidget(newDocButton, 1, 1);
	connect(newDocButton, SIGNAL(clicked()), this, SLOT(openDocument()));

	KGuiItem cancelItem(i18n("Cancel"), "dialog-cancel");
	KPushButton *cancelButton = new KPushButton(cancelItem, this);
	layout->addWidget(cancelButton, 3, 1);
	connect(cancelButton, SIGNAL(clicked()), m_pViewArea, SLOT(deleteLater()));
}

EmptyViewArea::~EmptyViewArea()
{
}

void EmptyViewArea::openDocument() {
	KTechlab::self()->openFile(m_pViewArea);
}
//END class EmptyViewArea

#include "moc_viewcontainer.cpp"
