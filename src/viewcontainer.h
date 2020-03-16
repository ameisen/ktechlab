#pragma once

#include "pch.hpp"

#include <QMap>
#include <QSplitter>
#include <QList>
#include <QPointer>

class KConfigGroup;
class KTechlab;
class View;
class ViewArea;
class ViewContainer;

class KConfig;
class QHBoxLayout;
class QLayout;
class QSplitter;

using ViewAreaMap = QMap<int, ViewArea *>;
using IntList = QList<int>;

/**
Before a ViewAre has been given a view, this is shown.
\author David Saxton
*/
class EmptyViewArea final : public QWidget {
	Q_OBJECT
	public:
		EmptyViewArea(ViewArea *parent);
		~EmptyViewArea() override;

	protected slots:
		void openDocument();

	protected:
		ViewArea * const m_pViewArea = nullptr;
};

/**
Contains either exactly one View, or two ViewAreas, separated by a QSplitter.
If it contains one view, then the value returned in id() is that of the view.
If it contains two ViewAreas, then the value returned by id() is -1.
@author David Saxton
*/
class ViewArea final : public QSplitter {
Q_OBJECT
public:
	enum class Position {
		Right,
		Bottom
	};

	ViewArea(QWidget *parent, ViewContainer *viewContainer, int id, bool showOpenButton, const char *name = nullptr);
	~ViewArea() override;

	ViewContainer *viewContainer() const { return p_viewContainer; }
	int id() const { return m_id; }

	/**
	 * Splits this ViewArea into two, and returns a pointer to the new ViewArea
	 * @param showOpenButton Whether to present the user with the EmptyViewArea
	 * widget (i.e. the new ViewArea is not destined to be immediately filled
	 * with a view).
	 */
	ViewArea *createViewArea(Position position, int id, bool showOpenButton);
	/**
	 * Adds the given View to the main part of the layout
	 */
	void setView(View *view);
	/**
	 * Saves the state of this ViewArea and any contained ViewAreas
	 */
	void saveState(KConfigGroup *config);
	/**
	 * Restores the state of this ViewArea and any contained ViewAreas
	 * @param groupName e.g. "ViewContainer 1"
	 */
	void restoreState(KConfigGroup *config, int id, const QString &groupName);
	/**
	 * Returns true if this ViewArea can save useful information as to its state
	 * (i.e. it's children can save useful information about their state, or has
	 * a view with a url in it)
	 */
	bool canSaveUsefulStateInfo() const;

	static QString fileKey(int id);
	static QString containsKey(int id);
	static QString orientationKey(int id);

	View * view() const { return p_view; }

protected slots:
	void viewAreaDestroyed(QObject *obj);
	void viewDestroyed();

protected:
	EmptyViewArea *m_pEmptyViewArea = nullptr;
	QPointer<View> p_view;
	ViewArea *p_viewArea[2] = { nullptr, nullptr };
	ViewContainer *p_viewContainer;

	int m_id = 0;
};

/**
@author David Saxton
*/
class ViewContainer final : public QWidget {
Q_OBJECT
public:
	/**
	 * Constructs a new ViewContainer, along with a default ViewArea ready for
	 * parenting a View, with an id of 0. parent is only used if ktechlab is
	 * null; otherwise the parent widget is ktechlab's tabWidget()
	 */
	ViewContainer(const QString & caption, QWidget *parent = nullptr);
	~ViewContainer() override;

	/**
	 * Returns the view in the ViewArea with the given id
	 */
	View *view(int id) const;
	/**
	 * Returns the ViewArea with the given id
	 */
	ViewArea *viewArea(int id) const;
	/**
	 * The active view area is the one that is focused.
	 */
	void setActiveViewArea(int id);
	/**
	 * Returns the id of the active ViewArea
	 */
	int activeViewArea() const { return m_activeViewArea; }
	/**
	 * Returns a pointer to the view of the active view area
	 */
	View *activeView() const { return view(activeViewArea()); }
	/**
	 * Attempts to close the given viewarea, returning true if successful (i.e
	 * if the user did not object to the close request )
	 */
	bool closeViewArea(int id);
	/**
	 * Creates a view area (parent QWidget, splitter et al) ready for inclusion
	 * of a view.
	 * @param relativeViewArea the viewarea to position the new viewarea next to, if -1 then is taken to be the active view area
	 * @param position Top, Right, Bottom or Left of given relativeViewArea
	 * @returns id of the view area, or -1 if unsucessful
	 * @param showOpenButton Whether to present the user with the EmptyViewArea
	 * widget (i.e. the new ViewArea is not destined to be immediately filled
	 * with a view).
	 */
	int createViewArea(int relativeViewArea, ViewArea::Position position, bool showOpenButton);
	/**
	 * Attempts to close each view area, returning false if any fail to be
	 * closed
	 */
	bool closeViewContainer();
	/**
	 * @returns number of views in this view container
	 */
	int viewCount() const { return m_viewAreaMap.size(); }
	/**
	 * Sets the pointer to the view area with the given id
	 */
	void setViewAreaId(ViewArea *viewArea, int id);
	/**
	 * Removes a ViewArea from internal lists
	 */
	void setViewAreaRemoved(int id);
	/**
	 * Sets the id to be "used"
	 */
	void setIdUsed(int id);
	/**
	 * Writes the state of the View Container (layout of views and view URLs)
	 * to the given KConfig. Doesn't change the group - so preset it if
	 * needed!
	 */
	void saveState(KConfigGroup *config);
	/**
	 * Reads in the saved config state (as written by saveState), and restores
	 * the ViewContainer with all appropriate views open
	 * @param groupName e.g. "ViewContainer 1"
	 */
	void restoreState(KConfigGroup *config, const QString &groupName);
	/**
	 * Returns a unique id (negative) for a ViewArea that is now a Parent of other ViewAreas
	 */
	int uniqueParentId();
	/**
	 * Returns a unique id (positive) for a new ViewArea
	 */
	int uniqueNewId();
	/**
	 * Returns true if this ViewArea can save useful information as to its state
	 * (i.e. it's children can save useful information about their state, or has
	 * a view with a url in it)
	 */
	bool canSaveUsefulStateInfo() const;

public slots:
	/**
	 * Sets the tab caption et al from the contents of this ViewContainer
	 */
	void updateCaption();

protected slots:
	void baseViewAreaDestroyed( QObject *obj );

protected:
	void restoreViewArea( KConfigGroup *config, int id, ViewArea *viewArea );
	void findActiveViewArea();
	ViewAreaMap m_viewAreaMap;
	IntList m_usedIDs;
	ViewArea *m_baseViewArea = nullptr;
	int m_activeViewArea = 0;
	bool b_deleted = false;
};
