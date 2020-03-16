#pragma once

#include "pch.hpp"

#include <map>
#include <memory>

#include "ktlqt3support/ktlq3scrollview.h"
#include <QPixmap>
#include <QBrush>
#include <QPen>
#include <QList>

#include "canvasitemlist.h"

class KtlQCanvasView;
class KtlQCanvasChunk;

class KtlQCanvas : public QObject {
	Q_OBJECT
	public:
		KtlQCanvas(QObject *parent = nullptr, const char *name = nullptr);
		KtlQCanvas(int w, int h, int chunksze = 16, int maxclust = 100);
		KtlQCanvas(const QPixmap &p, int h, int v, int tilewidth, int tileheight);

		~KtlQCanvas() override;

		virtual void setTiles(
			const QPixmap &tiles,
			int h,
			int v,
			int tilewidth,
			int tileheight
		);

		virtual void setBackgroundPixmap( const QPixmap &p );
		const QPixmap & backgroundPixmap() const;

		virtual void setBackgroundColor( const QColor &c );
		const QColor & backgroundColor() const;

		virtual void setTile( int x, int y, int tilenum );
		int tile( int x, int y ) const { return grid.get()[ x + (y * htiles)]; }

		int tilesHorizontally() const { return htiles; }
		int tilesVertically() const { return vtiles; }

		int tileWidth()  const { return tilew; }
		int tileHeight() const { return tileh; }

		virtual void resize( const QRect &newSize );
		int width()  const { return size().width(); }
		int height() const { return size().height(); }
		QSize size() const { return m_size.size(); }
		const QRect & rect() const { return m_size; }
		bool onCanvas( const int x, const int y ) const { return onCanvas( QPoint( x, y ) ); }
		bool onCanvas( const QPoint &p ) const { return m_size.contains( p ); }
		bool validChunk( const int x, const int y ) const { return validChunk( QPoint( x, y ) ); }
		bool validChunk( const QPoint &p ) const { return m_chunkSize.contains( p ); }

		int chunkSize() const { return chunksize; }
		virtual void retune(int chunksize, int maxclusters = 100);
		virtual void setChangedChunk(int i, int j);
		virtual void setChangedChunkContaining(int x, int y);
		virtual void setAllChanged();
		virtual void setChanged(const QRect &area);
		virtual void setUnchanged(const QRect &area);

		// These call setChangedChunk.
		void addItemToChunk(KtlQCanvasItem *, int i, int j);
		void removeItemFromChunk(KtlQCanvasItem *, int i, int j);
		void addItemToChunkContaining(KtlQCanvasItem *, int x, int y);
		void removeItemFromChunkContaining(KtlQCanvasItem *, int x, int y);

		KtlQCanvasItemList allItems() const;
		QList<KtlQCanvasItem *> collisions( const QPoint & ) /* const */ ;
		QList<KtlQCanvasItem *> collisions( const QRect & ) /* const */;
		QList<KtlQCanvasItem *> collisions(
			const QPolygon &pa,
			const KtlQCanvasItem *item,
			bool exact
		) const;

		void drawArea(const QRect &, QPainter *p);

		// These are for KtlQCanvasView to call
		virtual void addView(KtlQCanvasView *);
		virtual void removeView(KtlQCanvasView *);
		void drawCanvasArea(const QRect &, QPainter *p);
		void drawViewArea( KtlQCanvasView *view, QPainter *p, const QRect &r );

		// These are for KtlQCanvasItem to call
		virtual void addItem(KtlQCanvasItem *);
		virtual void removeItem(const KtlQCanvasItem *);

		virtual void setUpdatePeriod(int ms);
		int toChunkScaling(int x) const {
			return toChunkScaling(x, chunksize);
		}
		static int toChunkScaling(int x, int chunkSize);

	signals:
		void resized();

	public slots:
		virtual void advance();
		virtual void update();

	protected:
		virtual void drawBackground(QPainter &, const QRect &area);
		virtual void drawForeground(QPainter &, const QRect &area);

	private:
		QRect getChunkSize(const QRect &s) const {
			return getChunkSize(s, chunksize);
		}
		static QRect getChunkSize(const QRect &s, int chunkSize);

		KtlQCanvasChunk & chunk(int i, int j) const;
		KtlQCanvasChunk & chunkContaining(int x, int y) const;

		QRect changeBounds(const QRect &inarea);
		void drawChanges(const QRect &inarea);
		void drawChangedItems( QPainter &painter );
		void setNeedRedraw( const KtlQCanvasItemList *list );

		void initTiles(const QPixmap &p, int h, int v, int tilewidth, int tileheight);

		SortedCanvasItems m_canvasItems;
		QList<KtlQCanvasView *> m_viewList;
		QPixmap offscr;
		QPixmap pm;
		QRect m_size;
		QRect m_chunkSize;
		std::unique_ptr<KtlQCanvasChunk[]> chunks;
		QTimer *update_timer = nullptr;
		std::unique_ptr<ushort[]> grid;
		QColor bgcolor = Qt::white;
		int chunksize = 0;
		int maxclusters = 0;

		ushort htiles = 0;
		ushort vtiles = 0;
		ushort tilew = 0;
		ushort tileh = 0;
		bool oneone = false;
		bool debug_redraw_areas = false;

		friend void qt_unview(KtlQCanvas *c);

		KtlQCanvas(const KtlQCanvas &) = delete;
		KtlQCanvas &operator = (const KtlQCanvas &) = delete;
};

class KtlQCanvasViewData;

class KtlQCanvasView : public KtlQ3ScrollView {
	Q_OBJECT
	public:
		KtlQCanvasView(QWidget *parent = nullptr, const char *name = nullptr, Qt::WFlags f = 0); // 2018.08.15 - unused?
		KtlQCanvasView(KtlQCanvas *viewing, QWidget *parent = nullptr, const char *name = nullptr, Qt::WFlags f = 0);
		~KtlQCanvasView() override;

		KtlQCanvas * canvas() const { return viewing; }
		void setCanvas(KtlQCanvas *v);

		const QMatrix & worldMatrix() const;
		const QMatrix & inverseWorldMatrix() const;
		bool setWorldMatrix( const QMatrix & );

	protected:
    /** overrides KtlQ3ScrollView::drawContents() */   // override paintEvent?
		void drawContents( QPainter *, int cx, int cy, int cw, int ch ) override;
		QSize sizeHint() const override;

	private:
		void drawContents( QPainter * ) override;
		friend void qt_unview(KtlQCanvas* c);
		KtlQCanvasView( const KtlQCanvasView & ) = delete;
		KtlQCanvasView & operator = ( const KtlQCanvasView & ) = delete;
		KtlQCanvas *viewing = nullptr;
		KtlQCanvasViewData *d = nullptr;

	private slots:
		void cMoving(int, int);
		void updateContentsSize();
};
