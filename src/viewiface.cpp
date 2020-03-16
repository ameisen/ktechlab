#include "circuitview.h"
#include "document.h"
#include "flowcodeview.h"
#include "mechanicsview.h"
#include "textview.h"
#include "viewiface.h"

//BEGIN class ViewIface
DCOPRef ViewIface::document() const {
	return DCOPRef();
}

bool ViewIface::hasFocus() const {
	return m_pView->hasFocus();
}

bool ViewIface::close() {
	return m_pView->closeView();
}

void ViewIface::zoomIn() {
	m_pView->viewZoomIn();
}

void ViewIface::zoomOut() {
	m_pView->viewZoomOut();
}

bool ViewIface::canZoomIn() const {
	return m_pView->canZoomIn();
}

bool ViewIface::canZoomOut() const {
	return m_pView->canZoomOut();
}

void ViewIface::actualSize() {
	m_pView->actualSize();
}

// hack
double ViewIface::zoomLevelItemView() const {
	return static_cast<ItemView *>(m_pView)->zoomLevel();
}
//END class ViewIface

//BEGIN class TextViewIface
void TextViewIface::toggleBreakpoint() {
	getView()->toggleBreakpoint();
}

bool TextViewIface::gotoLine(const int line) {
	return getView()->gotoLine(line);
}
//END class TextViewIface
