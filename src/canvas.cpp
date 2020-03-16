#include "utils.h"
#include "canvas.h"
#include "canvas_private.h"

#include <QDebug>
#include <QApplication>
#include <QBitmap>
#include <QPainter>
#include <QTimer>
#include <QDesktopWidget>
#include <QSet>

#include "ktlq3polygonscanner.h"
#include <ktlqt3support/ktlq3scrollview.h>

#include <numeric>

namespace {
	static constexpr const bool isCanvasDebugEnabled = false;
}

//BEGIN class KtlQCanvasClusterizer

/*
A KtlQCanvasClusterizer groups rectangles (QRects) into non-overlapping rectangles
by a merging heuristic.
*/
KtlQCanvasClusterizer::KtlQCanvasClusterizer(int maxClusters) :
	clusters(std::make_unique<QRect[]>(maxClusters)),
	capacity(maxClusters)
{ }

void KtlQCanvasClusterizer::clear() {
	count_ = 0;
}

void KtlQCanvasClusterizer::add(int x, int y) {
	add(QPointRect{x, y});
}

void KtlQCanvasClusterizer::add(const QPoint &point) {
	add(QPointRect{point});
}

void KtlQCanvasClusterizer::add(int x, int y, int w, int h) {
	add(QRect(x, y, w, h));
}

namespace {
	template <typename... Tt>
	// TODO : Add enable_if to validate that it's a QRect
	static int GetCost(const QRect &rect, const Tt &... rects) {
		// This should end up being the area of rect - sum(area(rects))
		// Which is cost - cost1 - cost2 - cost3...
		int cost = GetArea(rect);
		((cost -= GetArea(rects)), ...);
		return cost;
	}
}

void KtlQCanvasClusterizer::add(const QRect &rect) {
	const auto biggerRect = rect.adjusted(
		-1, -1,
		2, 2
	);

	for (int cursor : Times{count_}) {
		if (clusters.get()[cursor].contains(rect)) {
			// Already wholly contained.
			return;
		}
	}

	const auto resetCosts = []() -> std::pair<int, int> {
		return { MaxValue<int>, -1 };
	};

	{
		auto [lowestCost, cheapest] = resetCosts();

		for (int cursor : Times{count_}) {
			if (!clusters.get()[cursor].intersects(biggerRect)) {
				continue;
			}

			const auto &clusterRect = clusters.get()[cursor];
			QRect larger = clusterRect | rect;
			const int cost = GetCost(larger, clusterRect);

			if (cost >= lowestCost) {
				continue;
			}

			bool bad = false;
			for (int c : Times{count_}) {
				if (c != cursor && clusters.get()[c].intersects(larger)) {
					bad = true;
					break;
				}
			}

			if (!bad) {
				cheapest = cursor;
				lowestCost = cost;
			}
		}

		if (cheapest >= 0) {
			clusters.get()[cheapest] |= rect;
			return;
		}

		if (count_ < capacity) {
			clusters.get()[count_++] = rect;
			return;
		}
	}

  // Do cheapest of:
  //     add to closest cluster
	//     do cheapest cluster merge, add to new cluster

	int lastCheapest = -1;
	{
		auto [lowestCost, cheapest] = resetCosts();

		for (int cursor : Times{count_}) {
			const auto &clusterRect = clusters.get()[cursor];

			QRect larger = clusterRect | rect;
			const int cost = GetCost(larger, clusterRect);

			if (cost >= lowestCost) {
				continue;
			}

			bool bad = false;

			for (int c : Times{count_}) {
				if (c != cursor && clusters.get()[c].intersects(larger)) {
					bad = true;
					break;
				}
			}

			if (!bad) {
				cheapest = cursor;
				lowestCost = cost;
			}
		}

		lastCheapest = cheapest;
	}

  // ###
  // could make an heuristic guess as to whether we need to bother
  // looking for a cheap merge.

	int lowestCost = MaxValue<int>;
	int cheapestMerge1 = -1;
	int cheapestMerge2 = -1;

	for (int merge1 : Times{count_}) {
		const auto &clusterRect1 = clusters.get()[merge1];

		for (int merge2 : Times{count_}) {
			if (merge1 == merge2) continue;

			const auto &clusterRect2 = clusters.get()[merge2];

			QRect larger = clusterRect1 | clusterRect2;

			const int cost = GetCost(larger, clusterRect1, clusterRect2);

			if (cost >= lowestCost) {
				continue;
			}

			bool bad = false;

			for (int c : Times{count_}) {
				if (c != merge1 && c != merge2 && clusters.get()[c].intersects(larger)) {
					bad = true;
					break;
				}
			}

			if (!bad) {
				cheapestMerge1 = merge1;
				cheapestMerge2 = merge2;
				lowestCost = cost;
			}
		}
	}

	if (cheapestMerge1 >= 0) {
		clusters.get()[cheapestMerge1] |= clusters.get()[cheapestMerge2];
		clusters.get()[cheapestMerge2] = clusters.get()[count_--];
	}
	else {
		clusters.get()[lastCheapest] |= rect;
	}

	// NB: clusters do not intersect (or intersection will
	//     overwrite). This is a result of the above algorithm,
	//     given the assumption that (x,y) are ordered topleft
	//     to bottomright.
	// add explicit x/y ordering to that comment, move it to the top
	// and rephrase it as pre-/post-conditions.
}

//END class KtlQCanvasClusterizer

int KtlQCanvas::toChunkScaling(int x, int chunkSize) {
	return roundDown(x, chunkSize);
}

QRect KtlQCanvas::getChunkSize(const QRect &s, int chunkSize) {
	return QRect{
		toChunkScaling(s.left(), chunkSize),
		toChunkScaling(s.top(), chunkSize),
		((s.width() - 1) / chunkSize) + 3,
		((s.height() - 1) / chunkSize) + 3
	};
}

KtlQCanvas::KtlQCanvas(int w, int h, int chunksze, int maxclust) :
	QObject(),
	m_size(QRect{0, 0, w, h}),
	m_chunkSize(getChunkSize(m_size, chunksze)),
	chunks(std::make_unique<KtlQCanvasChunk[]>(GetArea(m_chunkSize))),
	chunksize(chunksze),
	maxclusters(maxclust)
{}

KtlQCanvas::KtlQCanvas([[maybe_unused]] QObject *parent, const char *name) :
	KtlQCanvas(0, 0)
{
	setObjectName(name);
}

KtlQCanvas::KtlQCanvas(const QPixmap &p, int h, int v, int tilewidth, int tileheight) :
	KtlQCanvas(
		h * tilewidth,
		v * tileheight,
		std::lcm(tilewidth, tileheight)
	)
{
	setTiles(p, h, v, tilewidth, tileheight);
}

void qt_unview(KtlQCanvas *c) {
	for (auto &view : c->m_viewList) {
		view->viewing = nullptr;
	}
}

KtlQCanvas::~KtlQCanvas() {
	qt_unview(this);
	for (auto &item : allItems()) {
		if (!item) continue;
		delete item;
	}
	delete update_timer;
}

/*!
	\internal
	Returns the chunk at a chunk position \a i, \a j.
 */
KtlQCanvasChunk& KtlQCanvas::chunk(int i, int j) const {
	i -= m_chunkSize.left();
	j -= m_chunkSize.top();

  const int chunkOffset = i + m_chunkSize.width() * j;
  if (chunkOffset < 0 || chunkOffset >= GetArea(m_chunkSize)) {
  	qWarning() << Q_FUNC_INFO << " invalid chunk coordinates: " << i << " " << j;
    return chunks.get()[0]; // at least it should not crash
  }
	return chunks.get()[chunkOffset];
}

/*!
	\internal
	Returns the chunk at a pixel position \a x, \a y.
 */
KtlQCanvasChunk& KtlQCanvas::chunkContaining(int x, int y) const {
	return chunk(
		toChunkScaling(x),
		toChunkScaling(y)
	);
}

// TODO : this is dumb
KtlQCanvasItemList KtlQCanvas::allItems() const {
	KtlQCanvasItemList list;
	list.reserve(m_canvasItems.size());
	for (auto &itemPair : m_canvasItems) {
		auto &item = itemPair.second;
		if (!item) continue;
		list << item;
	}
	return list;
}

void KtlQCanvas::resize(const QRect &newSize) {
	if (newSize == m_size)
		return;

	QList<KtlQCanvasItem *> hidden;
	hidden.reserve(m_canvasItems.size());
	for (auto &itemPair : m_canvasItems) {
		auto &item = itemPair.second;
		if (!item) continue;
		if (!item->isVisible()) continue;
		item->hide();
		hidden.append(item);
	}

	m_chunkSize = getChunkSize(newSize);
	chunks = std::make_unique<KtlQCanvasChunk[]>(GetArea(m_chunkSize));
	m_size = newSize;

	for (auto *item : hidden) {
		item->show();
	}

	setAllChanged();

	emit resized();
}

void KtlQCanvas::retune(int chunksze, int mxclusters) {
	maxclusters = mxclusters;

	if (chunksize == chunksze) {
		return;
	}

	QList<KtlQCanvasItem *> hidden;
	hidden.reserve(m_canvasItems.size());
	for (auto &itemPair : m_canvasItems) {
		auto &item = itemPair.second;
		if (!item) continue;
		if (!item->isVisible()) continue;
		item->hide();
		hidden.append(item);
	}

	chunksize = chunksze;

	m_chunkSize = getChunkSize(m_size);
	chunks = std::make_unique<KtlQCanvasChunk[]>(GetArea(m_chunkSize));

	for (auto *item : hidden) {
		item->show();
	}
}

void KtlQCanvas::addItem(KtlQCanvasItem *item) {
	m_canvasItems.insert({item->z(), item});
}

void KtlQCanvas::removeItem(const KtlQCanvasItem *item) {
	for (auto it = m_canvasItems.begin(); it != m_canvasItems.end();) {
		auto &itItem = it->second;
		if (!itItem || itItem == item) {
			it = m_canvasItems.erase(it);
		}
		else {
			++it;
		}
	}
}

void KtlQCanvas::addView(KtlQCanvasView *view) {
	// TODO : Check if it already contains it?
	m_viewList.append(view);
	if (htiles > 1 || vtiles > 1 || pm.isNull()) {
  	QPalette palette;
  	palette.setColor(view->viewport()->backgroundRole(), backgroundColor());
  	view->viewport()->setPalette(palette);
	}
}

void KtlQCanvas::removeView(KtlQCanvasView *view) {
  m_viewList.removeAll(view);
}

void KtlQCanvas::setUpdatePeriod(int ms) {
	if (ms < 0) {
		if (update_timer) {
			update_timer->stop();
		}
	}
	else {
		delete update_timer;
		update_timer = new QTimer(this);
		connect(update_timer, SIGNAL(timeout()), this, SLOT(update()));
		update_timer->start(ms);
	}
}

// Don't call this unless you know what you're doing.
// p is in the content's co-ordinate example.
void KtlQCanvas::drawViewArea(KtlQCanvasView *view, QPainter *p, const QRect &vr) {
	QPoint tl = view->contentsToViewport(QPoint{0, 0});

	QMatrix wm = view->worldMatrix();
	QMatrix iwm = wm.inverted();
    // ivr = covers all chunks in vr
	QRect ivr = iwm.mapRect(vr);
	QMatrix twm;
	twm.translate(tl.x(), tl.y());

	QRect all = m_size;

  if (!p->isActive()) {
  	qWarning() << Q_FUNC_INFO << " painter is not active";
  }

	if (!all.contains(ivr)) {
		// Need to clip with edge of canvas.

		// For translation-only transformation, it is safe to include the right
		// and bottom edges, but otherwise, these must be excluded since they
		// are not precisely defined (different bresenham paths).
		QPolygon a;
		if (
			wm.m12() == 0.0 &&
			wm.m21() == 0.0 &&
			wm.m11() == 1.0 &&
			wm.m22() == 1.0
		) {
			a = QPolygon(QRect(
				all.x(),
				all.y(),
				all.width() + 1,
				all.height() + 1
			));
		}
		else {
			a = QPolygon(all);
		}

		a = (wm * twm).map(a);

    QWidget *vp = view->viewport();
    if (vp->palette().color(vp->backgroundRole()) == QColor(Qt::transparent)) {
			QRect cvr = vr;
			cvr.translate(tl.x(), tl.y());
			p->setClipRegion(QRegion(cvr) - QRegion(a));
			p->fillRect(
				vr,
				view->viewport()->palette().brush(QPalette::Active, QPalette::Window)
			);
		}
		p->setClipRegion(a);
	}

	p->setWorldMatrix(wm * twm);

	p->setBrushOrigin(tl.x(), tl.y());
	drawCanvasArea(ivr, p);
}

void KtlQCanvas::advance() {
  qWarning() << "KtlQCanvas::advance: TODO"; // TODO
}

/*!
	Repaints changed areas in all views of the canvas.
 */
void KtlQCanvas::update() {
	KtlQCanvasClusterizer clusterizer(m_viewList.count());
	QList<QRect> doneRects;

	for (auto *view : m_viewList) {
		QMatrix wm = view->worldMatrix();

		// TODO : There's probably a better way to do this.
		QRect area(
			view->contentsX(),
			view->contentsY(),
			view->visibleWidth(),
			view->visibleHeight()
		);

		if (area.width() <= 0 || area.height() <= 0) {
			continue;
		}

		if (!wm.isIdentity()) {
			// r = Visible area of the canvas where there are changes
			QRect r = changeBounds(view->inverseWorldMatrix().mapRect(area));
			if (!r.isEmpty()) {
				// as of my testing, drawing below always fails, so just post for an update event to the widget
				view->viewport()->update();
				doneRects.append(r);
			}
		}
		else {
			clusterizer.add(area);
		}
	}

	for (int i : Times{clusterizer.count()}) {
		drawChanges(clusterizer[i]);
	}

	for (auto &doneRect : doneRects) {
		setUnchanged(doneRect);
	}
}

/*!
	Marks the whole canvas as changed.
	All views of the canvas will be entirely redrawn when
	update() is called next.
 */
void KtlQCanvas::setAllChanged() {
	setChanged(m_size);
}

/*!
	Marks \a area as changed. This \a area will be redrawn in all
	views that are showing it when update() is called next.
 */
void KtlQCanvas::setChanged(const QRect &area) {
	const auto intersectedArea = area.intersect(m_size);

	const int mx = std::min(
		toChunkScaling(intersectedArea.x() + intersectedArea.width() + chunksize),
		m_chunkSize.right()
	);
	const int my = std::min(
		toChunkScaling(intersectedArea.y() + intersectedArea.height() + chunksize),
		m_chunkSize.bottom()
	);

	for (int x = toChunkScaling(intersectedArea.x()); x < mx; ++x) {
		for (int y = toChunkScaling(intersectedArea.y()); y < my; ++y) {
			chunk(x, y).change();
		}
	}
}

/*!
	Marks \a area as \e unchanged. The area will \e not be redrawn in
	the views for the next update(), unless it is marked or changed
	again before the next call to update().
 */
void KtlQCanvas::setUnchanged(const QRect &area) {
	const auto intersectedArea = area.intersect(m_size);

	const int mx = std::min(
		toChunkScaling(intersectedArea.x() + intersectedArea.width() + chunksize),
		m_chunkSize.right()
	);
	const int my = std::min(
		toChunkScaling(intersectedArea.y() + intersectedArea.height() + chunksize),
		m_chunkSize.bottom()
	);

	for (int x = toChunkScaling(intersectedArea.x()); x < mx; ++x) {
		for (int y = toChunkScaling(intersectedArea.y()); y < my; ++y) {
			chunk(x, y).takeChange();
		}
	}
}


QRect KtlQCanvas::changeBounds(const QRect &inarea) {
	const auto intersectedArea = inarea.intersect(m_size);

	const int mx = std::min(
		toChunkScaling(intersectedArea.x() + intersectedArea.width() + chunksize),
		m_chunkSize.right()
	);
	const int my = std::min(
		toChunkScaling(intersectedArea.y() + intersectedArea.height() + chunksize),
		m_chunkSize.bottom()
	);

	QRect result;

	for (int x = toChunkScaling(intersectedArea.x()); x < mx; ++x) {
		for (int y = toChunkScaling(intersectedArea.y()); y < my; ++y) {
			if (chunk(x, y).hasChanged()) {
				result |= QPointRect{x, y};
			}
		}
	}

	if (!result.isEmpty()) {
		result.setLeft( result.left() * chunksize );
		result.setTop( result.top() * chunksize );
		result.setRight( result.right() * chunksize );
		result.setBottom( result.bottom() * chunksize );
		// TODO : err, is this right?
		result.setRight( result.right() + chunksize );
		result.setBottom( result.bottom() + chunksize );
	}

	return result;
}

/*!
	Redraws the area \a inarea of the KtlQCanvas.
 */
void KtlQCanvas::drawChanges(const QRect &inarea) {
	const auto intersectedArea = inarea.intersect(m_size);

	const int mx = std::min(
		toChunkScaling(intersectedArea.x() + intersectedArea.width() + chunksize),
		m_chunkSize.right()
	);
	const int my = std::min(
		toChunkScaling(intersectedArea.y() + intersectedArea.height() + chunksize),
		m_chunkSize.bottom()
	);

	KtlQCanvasClusterizer clusters = {maxclusters};

	for (int x = toChunkScaling(intersectedArea.x()); x < mx; ++x) {
		for (int y = toChunkScaling(intersectedArea.y()); y < my; ++y) {
			if (chunk(x, y).hasChanged()) {
				clusters.add(x, y);
			}
		}
	}

	for (int i : Times{clusters.count()}) {
		QRect elarea = clusters[i];
		elarea.setRect(
			elarea.left() * chunksize,
			elarea.top() * chunksize,
			// TODO : err, is this right?
			(elarea.width() * chunksize) + chunksize,
			(elarea.height() * chunksize) + chunksize
		);
		drawCanvasArea(elarea, nullptr);
	}
}

/*!
	Paints all canvas items that are in the area \a clip to \a
	painter, using double-buffering if \a dbuf is true.

    e.g. to print the canvas to a printer:
	\code
	QPrinter pr;
	if ( pr.setup() ) {
	QPainter p(&pr);        // this code is in a comment block
	canvas.drawArea( canvas.rect(), &p );
}
	\endcode
 */
void KtlQCanvas::drawArea(const QRect &clip, QPainter *painter) {
	if (painter) {
		drawCanvasArea(clip, painter);
	}
}

void KtlQCanvas::drawCanvasArea(const QRect &inarea, QPainter *p) {
	if ((m_viewList.isEmpty() || !m_viewList.first()) && !p)
		return; // Nothing to do.

	const auto intersectedArea = inarea.intersect(m_size);

	const int mx = std::min(
		toChunkScaling(intersectedArea.x() + intersectedArea.width() + chunksize),
		m_chunkSize.right()
	);
	const int my = std::min(
		toChunkScaling(intersectedArea.y() + intersectedArea.height() + chunksize),
		m_chunkSize.bottom()
	);

  // Stores the region within area that need to be drawn. It is relative
  // to area.topLeft()  (so as to keep within bounds of 16-bit XRegions)
	QRegion rgn;

	for (int x = toChunkScaling(intersectedArea.x()); x < mx; ++x) {
		for (int y = toChunkScaling(intersectedArea.y()); y < my; ++y) {
			auto &currentChunk = chunk(x, y);

			if (!p) {
				if (currentChunk.takeChange()) {
					rgn |= QRegion(
						x * chunksize - intersectedArea.x(),
						y * chunksize - intersectedArea.y(),
						chunksize,
						chunksize
					);
					setNeedRedraw(currentChunk.listPtr());
				}
			}
			else {
				setNeedRedraw(currentChunk.listPtr());
			}
		}
	}

	if (p) {
		drawBackground(*p, intersectedArea);
		drawChangedItems(*p);
		drawForeground(*p, intersectedArea);
		return;
	}

	QPoint trtr; // keeps track of total translation of rgn

	trtr -= intersectedArea.topLeft();

	for (auto *view : m_viewList) {
		if (!view->worldMatrix().isIdentity())
			continue; // Cannot paint those here (see callers).

		// as of my testing, drawing below always fails, so just post for an update event to the widget
		view->viewport()->update();
	}
}

void KtlQCanvas::setNeedRedraw(const KtlQCanvasItemList *list) {
	if (!list) return;

	for (auto &item : *list) {
		if (!item) continue;
		item->setNeedRedraw(true);
	}
}


void KtlQCanvas::drawChangedItems(QPainter &painter) {
	for (auto &itemPair : m_canvasItems) {
		auto &item = itemPair.second;
		if (!item) continue;
		if (!item->needRedraw()) continue;
		item->draw(painter);
		item->setNeedRedraw(false);
	}
}

/*!
	\internal
	This method to informs the KtlQCanvas that a given chunk is
	`dirty' and needs to be redrawn in the next Update.

	(\a x,\a y) is a chunk location.

	The sprite classes call this. Any new derived class of KtlQCanvasItem
	must do so too. SetChangedChunkContaining can be used instead.
 */
void KtlQCanvas::setChangedChunk(int x, int y) {
	if (validChunk(x,y)) {
		chunk(x, y).change();
	}
}

/*!
	\internal
	This method to informs the KtlQCanvas that the chunk containing a given
	pixel is `dirty' and needs to be redrawn in the next Update.

	(\a x,\a y) is a pixel location.

	The item classes call this. Any new derived class of KtlQCanvasItem must
	do so too. SetChangedChunk can be used instead.
 */
void KtlQCanvas::setChangedChunkContaining(int x, int y) {
	if (onCanvas(x, y)) {
		chunkContaining(x, y).change();
	}
}

/*!
	\internal
	This method adds the KtlQCanvasItem \a g to the list of those which need to be
	drawn if the given chunk at location ( \a x, \a y ) is redrawn. Like
	SetChangedChunk and SetChangedChunkContaining, this method marks the
	chunk as `dirty'.
 */
void KtlQCanvas::addItemToChunk(KtlQCanvasItem *g, int x, int y) {
	if (validChunk(x, y)) {
		chunk(x, y).add(g);
	}
}

/*!
	\internal
	This method removes the KtlQCanvasItem \a g from the list of those which need to
	be drawn if the given chunk at location ( \a x, \a y ) is redrawn. Like
	SetChangedChunk and SetChangedChunkContaining, this method marks the chunk
	as `dirty'.
 */
void KtlQCanvas::removeItemFromChunk(KtlQCanvasItem *g, int x, int y) {
	if (validChunk(x,y)) {
		chunk(x, y).remove(g);
	}
}

/*!
	\internal
	This method adds the KtlQCanvasItem \a g to the list of those which need to be
	drawn if the chunk containing the given pixel ( \a x, \a y ) is redrawn. Like
	SetChangedChunk and SetChangedChunkContaining, this method marks the
	chunk as `dirty'.
 */
void KtlQCanvas::addItemToChunkContaining(KtlQCanvasItem *g, int x, int y) {
	if (onCanvas(x, y)) {
		chunkContaining(x, y).add(g);
	}
}

/*!
	\internal
	This method removes the KtlQCanvasItem \a g from the list of those which need to
	be drawn if the chunk containing the given pixel ( \a x, \a y ) is redrawn.
	Like SetChangedChunk and SetChangedChunkContaining, this method
	marks the chunk as `dirty'.
 */
void KtlQCanvas::removeItemFromChunkContaining(KtlQCanvasItem *g, int x, int y) {
	if (onCanvas(x, y)) {
		chunkContaining(x, y).remove(g);
	}
}

/*!
	Returns the color set by setBackgroundColor(). By default, this is
	white.

	This function is not a reimplementation of
	QWidget::backgroundColor() (KtlQCanvas is not a subclass of QWidget),
	but all QCanvasViews that are viewing the canvas will set their
	backgrounds to this color.

	\sa setBackgroundColor(), backgroundPixmap()
 */
const QColor & KtlQCanvas::backgroundColor() const {
	return bgcolor;
}

/*!
	Sets the solid background to be the color \a c.

	\sa backgroundColor(), setBackgroundPixmap(), setTiles()
 */
void KtlQCanvas::setBackgroundColor( const QColor &c ) {
	if (bgcolor == c) {
		return;
	}

	bgcolor = c;
	for (auto *view : m_viewList) {
		/* XXX this doesn't look right. Shouldn't this
				be more like setBackgroundPixmap? : Ian */
		QWidget *viewportWidget = view->viewport();
		QPalette palette;
		palette.setColor(viewportWidget->backgroundRole(), bgcolor);
		viewportWidget->setPalette(palette);
	}
	setAllChanged();
}

/*!
	Returns the pixmap set by setBackgroundPixmap(). By default,
	this is a null pixmap.

	\sa setBackgroundPixmap(), backgroundColor()
 */
const QPixmap & KtlQCanvas::backgroundPixmap() const {
	return pm;
}

/*!
	Sets the solid background to be the pixmap \a p repeated as
	necessary to cover the entire canvas.

	\sa backgroundPixmap(), setBackgroundColor(), setTiles()
 */
void KtlQCanvas::setBackgroundPixmap(const QPixmap &p) {
	setTiles(
		p,
		1,
		1,
		p.width(),
		p.height()
	);

	for (auto *view : m_viewList) {
		view->updateContents();
	}
}

/*!
	This virtual function is called for all updates of the canvas. It
	renders any background graphics using the painter \a painter, in
	the area \a clip. If the canvas has a background pixmap or a tiled
	background, that graphic is used, otherwise the canvas is cleared
	using the background color.

	If the graphics for an area change, you must explicitly call
	setChanged(const QRect&) for the result to be visible when
	update() is next called.

	\sa setBackgroundColor(), setBackgroundPixmap(), setTiles()
 */
void KtlQCanvas::drawBackground(QPainter &painter, const QRect &clip) {
	painter.fillRect(clip, Qt::white);

	if (pm.isNull()) {
		painter.fillRect(clip, bgcolor);
	}
	else if (!grid) {
		for (int x : Range{
			clip.x() / pm.width(),
			(clip.x() + clip.width() + pm.width() - 1) / pm.width()
		}) {
			for (int y : Range{
				clip.y() / pm.height(),
				(clip.y() + clip.height() + pm.height() - 1) / pm.height()
			}) {
				painter.drawPixmap(
					x * pm.width(),
					y * pm.height(),
					pm
				);
			}
		}
	}
	else {
		const int x1 = roundDown( clip.left(), tilew );
		const int x2 = roundDown( clip.right(), tilew ) + 1;
		const int y1 = roundDown( clip.top(), tileh );
		const int y2 = roundDown( clip.bottom(), tileh ) + 1;

		const int roww = pm.width() / tilew;

		for (int j : Range{y1, y2}) {
			const int tv = tilesVertically();
			const int jj = ((j % tv) + tv) % tv;
			for (int i : Range{x1, x2}) {
				const int th = tilesHorizontally();
				const int ii = ((i % th) + th) % th;
				const int t = tile(ii, jj);
				const int tx = t % roww;
				const int ty = t / roww;
				painter.drawPixmap(
					i * tilew,
					j * tileh,
					pm,
					tx * tilew,
					ty * tileh,
					tilew,
					tileh
				);
			}
		}
	}
}

void KtlQCanvas::drawForeground(QPainter &painter, const QRect &clip) {
	if (debug_redraw_areas) {
		painter.setPen(Qt::red);
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(clip);
	}
}

void KtlQCanvas::setTiles(const QPixmap &p, int h, int v, int tilewidth, int tileheight) {
	if (
		!p.isNull() &&
		(
			!tilewidth ||
			!tileheight ||
			(p.width() % tilewidth) != 0 ||
			(p.height() % tileheight) != 0
		)
	) {
		return;
	}

	htiles = h;
	vtiles = v;
	pm = p;
	if (h && v && !p.isNull()) {
		grid = std::make_unique<ushort[]>(h * v);
		memset(grid.get(), 0, h * v * sizeof(ushort));
		tilew = tilewidth;
		tileh = tileheight;
	}
	else {
		grid = nullptr;
	}
	if (h + v > 10) {
		const int s = std::lcm(tilewidth, tileheight);
		retune(
			(s < 128) ?
			s :
			qMax(tilewidth, tileheight)
		);
	}
	setAllChanged();
}

void KtlQCanvas::setTile(int x, int y, int tilenum) {
	assert(grid);

	ushort &t = grid.get()[x + y * htiles];
	if (t != tilenum) {
		t = tilenum;
		if (tilew == tileh && tilew == chunksize) {
			setChangedChunk(x, y); // common case
		}
		else {
			setChanged(QRect{
				x * tilew,
				y * tileh,
				tilew,
				tileh
			});
		}
	}
}

QList<KtlQCanvasItem *> KtlQCanvas::collisions(const QPoint &p) /* const */ {
	return collisions(QPointRect{p});
}

QList<KtlQCanvasItem *> KtlQCanvas::collisions(const QRect &r) /* const */ {
	auto i = KtlQCanvasRectangle{r, this}; // TODO verify here, why is crashing ?!
	i.setPen(QPen(Qt::NoPen));
	i.show(); // doesn't actually show, since we destroy it
	auto l = i.collisions(true);
	qSort(l);
	return l;
}


QList<KtlQCanvasItem *> KtlQCanvas::collisions(const QPolygon &chunklist, const KtlQCanvasItem *item, bool exact) const {
	if constexpr (isCanvasDebugEnabled) {
		qDebug() << Q_FUNC_INFO << " test item: " << item;
		for (auto &canvasItemPair : m_canvasItems) {
			auto &canvasItem = canvasItemPair.second;
			if (!canvasItem) continue;
			qDebug() << "   in canvas item: " << canvasItem;
		}
		qDebug() << "end canvas item list";
	}

  QSet<KtlQCanvasItem *> seen;
	QList<KtlQCanvasItem *> result;

	for (int i : Times{chunklist.count()}) {
		const auto &currentChunk = chunklist[i];
		const int x = currentChunk.x();
		const int y = currentChunk.y();
		if (!validChunk(x, y)) {
			continue;
		}
		const auto &l = chunk(x, y).getList();

		seen.reserve(seen.size() + l.size());
		result.reserve(result.size() + l.size());

		for (auto &canvasItem : l) {
			if (!canvasItem) continue;
			if (canvasItem == item) continue;

			if (seen.insert(canvasItem) == seen.end()) {
				continue;
			}

			if (!exact || item->collidesWith(canvasItem)) {
				result.append(canvasItem);
			}

			if constexpr (isCanvasDebugEnabled) {
				qDebug() <<"test collides " << item << " with " << canvasItem;
			}
		}
	}
	return result;
}

KtlQCanvasView::KtlQCanvasView(QWidget *parent, const char *name, Qt::WindowFlags f) :
	KtlQ3ScrollView(parent, name, f),
	d(new KtlQCanvasViewData)
{
	setAttribute(Qt::WA_StaticContents);
	setCanvas(nullptr);
	connect(this, SIGNAL(contentsMoving(int,int)), this, SLOT(cMoving(int, int)));
}

KtlQCanvasView::KtlQCanvasView(KtlQCanvas *canvas, QWidget *parent, const char *name, Qt::WindowFlags f) :
	KtlQ3ScrollView(parent, name, f),
	d(new KtlQCanvasViewData)
{
	setAttribute( Qt::WA_StaticContents );
	setCanvas(canvas);
	connect(this, SIGNAL(contentsMoving(int, int)), this, SLOT(cMoving(int, int)));
}

KtlQCanvasView::~KtlQCanvasView() {
	delete d;
	d = nullptr; // setCanvas checks this.
	setCanvas(nullptr);
}

void KtlQCanvasView::setCanvas(KtlQCanvas *canvas) {
	if (viewing) {
		disconnect(viewing);
		viewing->removeView(this);
	}
	viewing = canvas;
	if (viewing) {
		connect(viewing, SIGNAL(resized()), this, SLOT(updateContentsSize()));
		viewing->addView(this);
	}
	if (d) { // called by d'tor
		updateContentsSize();
	}
}

const QMatrix &KtlQCanvasView::worldMatrix() const {
	return d->xform;
}

const QMatrix &KtlQCanvasView::inverseWorldMatrix() const {
	return d->ixform;
}

bool KtlQCanvasView::setWorldMatrix(const QMatrix &wm) {
	if (!wm.isInvertible()) {
		return false;
	}
	d->xform = wm;
	d->ixform = wm.inverted();
	updateContentsSize();
	viewport()->update();
	return true;
}

void KtlQCanvasView::updateContentsSize() {
	if (!viewing) {
    viewport()->update();
		resizeContents(1, 1);
		return;
	}

	QRect br = d->xform.mapRect(viewing->rect());

	if (br.width() < contentsWidth()) {
		QRect r(
			contentsToViewport(QPoint(br.width(), 0)),
			QSize(contentsWidth() - br.width(), contentsHeight())
		);
		viewport()->update(r);
	}
	if (br.height() < contentsHeight()) {
		QRect r(
			contentsToViewport(QPoint(0, br.height())),
			QSize(contentsWidth(), contentsHeight() - br.height())
		);
		viewport()->update(r);
	}

	resizeContents(br.width(), br.height());
}

void KtlQCanvasView::cMoving(int x, int y) {
  // A little kludge to smooth up repaints when scrolling
	int dx = x - contentsX();
	int dy = y - contentsY();
	d->repaint_from_moving = (std::abs(dx) < (width() / 8)) && (std::abs(dy) < (height() / 8));
}

/*!
	Repaints part of the KtlQCanvas that the canvas view is showing
	starting at \a cx by \a cy, with a width of \a cw and a height of \a
	ch using the painter \a p.

	\warning When double buffering is enabled, drawContents() will
	not respect the current settings of the painter when setting up
	the painter for the double buffer (e.g., viewport() and
	window()). Also, be aware that KtlQCanvas::update() bypasses
	drawContents(), which means any reimplementation of
	drawContents() is not called.

	\sa setDoubleBuffering()
 */
void KtlQCanvasView::drawContents(QPainter *p, int cx, int cy, int cw, int ch) {
	QRect r = QRect{
		cx,
		cy,
		cw,
		ch
	}.normalized();

	if (viewing) {
		viewing->drawViewArea(this, p, r);
		d->repaint_from_moving = false;
	}
	else {
		p->eraseRect(r);
	}
}

/*!
	\reimp
	\internal

	(Implemented to get rid of a compiler warning.)
 */
void KtlQCanvasView::drawContents(QPainter *p) {
  qDebug() << Q_FUNC_INFO << " called unexpectedly";
  drawContents(p, 0, 0, width(), height());
}

/*!
	Suggests a size sufficient to view the entire canvas.
 */
QSize KtlQCanvasView::sizeHint() const {
	if (!canvas()) {
		return KtlQ3ScrollView::sizeHint(); // TODO QT3
	}
  // should maybe take transformations into account
	const auto canvasSize = canvas()->size() + 2 * QSize(frameWidth(), frameWidth());
	const auto canvasBounds = 3 * QApplication::desktop()->size() / 4;
	return canvasSize.boundedTo(canvasBounds);
}

#include "moc_canvas.cpp"
