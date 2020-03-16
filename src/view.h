#pragma once

#include "pch.hpp"

#include "viewcontainer.h"
#include "document.h"

#include <KStatusBar>
#include <KUrl>
#include <KXMLGUIClient>

#include <QPointer>
#include <QPixmap>
#include <QPainter>
#include <QEvent>
#include <QPalette>

class DCOPObject;
class Document;
class KSqueezedTextLabel;
class KTechlab;
class View;
class ViewContainer;
class ViewIface;

class KAction;

class QVBoxLayout;
class QLabel;

class ViewStatusBar final : public KStatusBar {
Q_OBJECT
public:
	ViewStatusBar(View *view);

	enum class InfoId : int {
		SimulationState = 0,
		LineCol,
		InsertMode,
		SelectionMode
	};

public slots:
	void slotModifiedStateChanged();
	void slotFileNameChanged(const KUrl &url);
	void slotViewFocused(View *);
	void slotViewUnfocused();

protected:
	QPixmap m_modifiedPixmap;
	QPixmap m_unmodifiedPixmap;
	View *p_view = nullptr;
	QLabel *m_modifiedLabel = nullptr;
	KSqueezedTextLabel *m_fileNameLabel = nullptr;
};

/**
@author David Saxton
*/
class View : public QWidget, public KXMLGUIClient {
Q_OBJECT
public:
	View(Document *document, ViewContainer *viewContainer, int viewAreaId, const char *name = nullptr);
	~View() override;

	QAction * actionByName(const QString &name) const;
	/**
	 * Pointer to the parent document
	 */
	Document * document() const { return m_pDocument; }
	/**
	 * Returns the DCOP object from this view
	 */
	DCOPObject * dcopObject() const;
	/**
	 * Returns the dcop suffix for this view - a unique ID for the current the
	 * view within all views associated with the parent document. DCOP name
	 * will become "View#docID#viewID".
	 */
	int dcopID() const { return m_dcopID; }
	/**
	 * Sets the dcop suffix. The DCOP object for this view will be renamed.
	 * @see dcopID
	 */
	void setDCOPID(int id);
	/**
	 * Pointer to the ViewContainer that we're in
	 */
	ViewContainer * viewContainer() const { return p_viewContainer; }
	/**
	 * Tells the view container which contains this view to close this view,
	 * returning true if successful (i.e. not both last view and unsaved, etc)
	 */
	virtual bool closeView();
	/**
	 * Returns the unique (for the view container) view area id associated with this view
	 */
	uint viewAreaId() const { return m_viewAreaId; }
	/**
	 * Zoom in
	 */
	virtual void viewZoomIn() {};
	/**
	 * Zoom out
	 */
	virtual void viewZoomOut() {};
	virtual bool canZoomIn() const { return true; }
	virtual bool canZoomOut() const { return true; }
	/**
	 * Restore view to actual size
	 */
	virtual void actualSize() {};

	virtual void toggleBreakpoint() {};
	bool eventFilter(QObject *watched, QEvent *e) override;

protected slots:
	/**
	 * Called when the user changes the configuration.
	 */
	virtual void slotUpdateConfiguration() {};

signals:
	/**
	 * Emitted when the view receives focus. @p view is a pointer to this class.
	 */
	void focused(View *view);
	/**
	 * Emitted when the view looses focus.
	 */
	void unfocused();

protected:
	/**
	 * This function should be called in the constructor of the child class
	 * (e.g. in ItemView or TextView) to set the widget which receives focus
	 * events.
	 */
	void setFocusWidget(QWidget *focusWidget);

	QPointer<Document> m_pDocument;
	QPointer<ViewContainer> p_viewContainer;
	ViewStatusBar *m_statusBar = nullptr;
	QVBoxLayout *m_layout = nullptr;
	ViewIface *m_pViewIface = nullptr;
	QWidget *m_pFocusWidget = nullptr;
	int m_viewAreaId = 0;
	int m_dcopID = 0;
};

/*
   "KateViewSpaceStatusBarSeparator"
   A 2 px line to separate the statusbar from the view.
   It is here to compensate for the lack of a frame in the view,
   I think Kate looks very nice this way, as QScrollView with frame
   looks slightly clumsy...
   Slight 3D effect. I looked for suitable QStyle props or methods,
   but found none, though maybe it should use QStyle::PM_DefaultFrameWidth
   for height (TRY!).
   It does look a bit funny with flat styles (Light, .Net) as is,
   but there are on methods to paint panel lines separately. And,
   those styles tends to look funny on their own, as a light line
   in a 3D frame next to a light contents widget is not functional.
   Also, QStatusBar is up to now completely ignorant to style.
   -anders
*/
class KVSSBSep final : public QWidget {
public:
	KVSSBSep(View *parent = nullptr) : QWidget(parent) {
		setFixedHeight(2);
	}

protected:
	void paintEvent(QPaintEvent *e) override;
};
