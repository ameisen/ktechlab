#pragma once

#include "pch.hpp"

#include "dcop_stub.h"

class CircuitView;
class FlowCodeView;
class ICNView;
class ItemView;
class MechanicsView;
class TextView;
class View;

/**
@author David Saxton
*/
class ViewIface : public DCOPObject {
	K_DCOP

	public:
		ViewIface(View *view) :
			DCOPObject(),
			m_pView(view)
		{}
		virtual ~ViewIface() = default;

	k_dcop:
		DCOPRef document() const;
		bool hasFocus() const;
		bool close();
		void zoomIn();
		void zoomOut();
		bool canZoomIn() const;
		bool canZoomOut() const;
		void actualSize();

	protected:
		View * const m_pView = nullptr;

		// hack until I figure out a better way to do this.
		double zoomLevelItemView() const;
};

template <typename T = View>
class ViewIfaceT : public ViewIface {
	K_DCOP

	public:
		ViewIfaceT(T *view) : ViewIface(reinterpret_cast<View *>(view)) {}

	protected:
		T * getView() { return reinterpret_cast<T *>(m_pView); }
		const T * getView() const { return reinterpret_cast<const T *>(m_pView); }
};

class TextViewIface final : public ViewIfaceT<TextView> {
	K_DCOP

	public:
		TextViewIface(TextView *view) : ViewIfaceT(view) {}

	k_dcop:
		void toggleBreakpoint();
		bool gotoLine(const int line);
};

template <typename T = ItemView>
class ItemViewIface : public ViewIfaceT<T> {
	K_DCOP

	public:
		ItemViewIface(T *view) : ViewIfaceT<T>(view) {}

	k_dcop:
		double zoomLevel() const {
			return ViewIface::zoomLevelItemView();
		}
};

using MechanicsViewIface = ItemViewIface<MechanicsView>;

template <typename T = ICNView>
using ICNViewIface = ItemViewIface<T>;

using CircuitViewIface = ICNViewIface<CircuitView>;
using FlowCodeViewIface = ICNViewIface<FlowCodeView>;
