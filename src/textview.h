#pragma once

#include "pch.hpp"

#include "config.h"
#include "view.h"

#include <KTextEditor/View>

#include <QPointer>
#include <QLabel>

class QMouseEvent;
class RegisterInfo;
class TextDocument;
class TextView;
class VariableLabel;

/**
@author David Saxton
*/
class TextView final : public View {
	Q_OBJECT
	public:
		TextView(TextDocument *textDocument, ViewContainer *viewContainer, int viewAreaId, const char *name = nullptr);
		~TextView() override;

		bool closeView() override;

		/**
		 * Brings up the goto line dialog.
		 */
		bool gotoLine(const int line);
		/**
		 * Returns a pointer to the document as a text document
		 */
		TextDocument *textDocument() const;
		void undo();
		void redo();
		void cut();
		void copy();
		void paste();
		void saveCursorPosition();
		void restoreCursorPosition();
		/**
		 * Enable code actions depending on the type of code being edited
		 */
		void initCodeActions();
		void setCursorPosition(int line, int col);
		int currentLine();
		int currentColumn();
		void disableActions();

		KTextEditor::View * kateView() const { return m_view; }

		bool save();
    bool saveAs();
  	void print();

	public slots:
		/**
		 * Called when change line / toggle marks
		 */
		void slotUpdateMarksInfo();
		void slotCursorPositionChanged();
		void toggleBreakpoint() override;
		/**
		 * Initialize the actions appropriate for when the debugger is running
		 * or stepping
		 */
		void slotInitDebugActions();

	protected slots:
		void slotWordHoveredOver(const QString &word, int line, int col);
		void slotWordUnhovered();
		void gotFocus();
    void slotSelectionmChanged();

	protected:
		KTextEditor::View *m_view = nullptr;
#ifndef NO_GPSIM
		VariableLabel *m_pTextViewLabel = nullptr; ///< Pops up when the user hovers his mouse over a word
#endif

	private:
		int m_savedCursorLine = 0;
		int m_savedCursorColumn = 0;
};

/**
This class is an event filter to be installed in the kate view, and is used to
do stuff like poping up menus and telling TextView that a word is being hovered
over (used in the debugger).

@author David Saxton
*/
class TextViewEventFilter final : public QObject {
	Q_OBJECT
	public:
		TextViewEventFilter(TextView *textView);

		bool eventFilter(QObject *watched, QEvent *e) override;

	signals:
		/**
		 * When the user hovers the mouse for more than 700 milliseconds over a
		 * word, "hover mode" is entered. When the user presses a key, clicks
		 * mouse, etc, this mode is left. During the mode, any word that is
		 * under the mouse cursor will be emitted as hoveredOver( word ).
		 */
		void wordHoveredOver(const QString &word, int line, int col);
		/**
		 * Emitted when focus is lost, the mouse moves to a different word, etc.
		 */
		void wordUnhovered();

	protected slots:
		void slotNeedTextHint(const KTextEditor::Cursor &position, const QString &text);
		/**
		 * Called when we are not in hover mode, but the user has had his mouse
		 * in the same position for some time.
		 */
		void hoverTimeout();
		/**
		 * Called (from m_pSleepTimer) when we are in hover mode, but no word
		 * has been hovered over for some time.
		 */
		void gotoSleep();
		/**
		 * @see m_pNoWordTimer
		 */
		void slotNoWordTimeout();

	protected:
		enum class HoverState : int {
			/**
			 * We are currently hovering - wordHoveredOver was emitted, and
			 * wordUnhovered hasn't been emitted yet.
			 */
			Active,
			/**
			 * A word was unhovered recently. We will go straight to PoppedUp
			 * mode if a word is hovered over again.
			 */
			Hidden,
			/**
			 * A word was not unhovered recentely. There will be a short display
			 * before going to PoppedUp mode if a word is hovered over.
			 */
			Sleeping
		};

		/**
		 * Starts / stops timers, emits signals, etc. See other functions for an
		 * idea of what this does.
		 */
		void updateHovering(const QString &currentWord, int line, int col);
		/**
		 * Started when the user moves his mouse over a word, and we are in
		 * Sleeping mode. Reset when the user moves his mouse, etc.
		 */
		QTimer *m_pHoverTimer = nullptr;
		/**
		 * Started when a word is unhovered. When this timeouts, we will go to
		 * Sleeping mode.
		 */
		QTimer *m_pSleepTimer = nullptr;
		/**
		 * Activated by the user moving the mouse. Reset by a call to
		 * slotNeedTextHint. This timer is needed as KateViewInternal doesn't
		 * bother updating us if the mouse cursor isn't over text.
		 */
		QTimer *m_pNoWordTimer = nullptr;

		TextView * m_pTextView = nullptr;
		QString m_lastWord;
		int m_lastLine = -1;
		int m_lastCol = -1;
		HoverState m_hoverStatus = HoverState::Sleeping;
};
