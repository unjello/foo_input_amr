class NoRedrawScope {
public:
	NoRedrawScope(HWND p_wnd) throw() : m_wnd(p_wnd) {
		m_wnd.SetRedraw(FALSE);
	}
	~NoRedrawScope() throw() {
		m_wnd.SetRedraw(TRUE);
	}
private:
	CWindow m_wnd;
};

class NoRedrawScopeEx {
public:
	NoRedrawScopeEx(HWND p_wnd) throw() : m_wnd(p_wnd), m_active() {
		if (m_wnd.IsWindowVisible()) {
			m_active = true;
			m_wnd.SetRedraw(FALSE);
		}
	}
	~NoRedrawScopeEx() throw() {
		if (m_active) {
			m_wnd.SetRedraw(TRUE);
			m_wnd.RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
		}
	}
private:
	bool m_active;
	CWindow m_wnd;
};

class CMenuSelectionReceiver : public CWindowImpl<CMenuSelectionReceiver> {
public:
	CMenuSelectionReceiver(HWND p_parent) {
		WIN32_OP( Create(p_parent) != NULL );
	}
	~CMenuSelectionReceiver() {
		if (m_hWnd != NULL) DestroyWindow();
	}
	typedef CWindowImpl<CMenuSelectionReceiver> _baseClass;
	DECLARE_WND_CLASS_EX(TEXT("{DF0087DB-E765-4283-BBAB-6AB2E8AB64A1}"),0,0);

	BEGIN_MSG_MAP(CMenuSelectionReceiver)
		MESSAGE_HANDLER(WM_MENUSELECT,OnMenuSelect)
	END_MSG_MAP()
protected:
	virtual bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return false;
	}
private:
	LRESULT OnMenuSelect(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
		if (p_lp != 0) {
			if (HIWORD(p_wp) & MF_POPUP) {
				m_status.release();
			} else {
				pfc::string8 msg;
				if (!QueryHint(LOWORD(p_wp),msg)) {
					m_status.release();
				} else {
					if (m_status.is_empty()) {
						if (!static_api_ptr_t<ui_control>()->override_status_text_create(m_status)) m_status.release();
					}
					if (m_status.is_valid()) {
						m_status->override_text(msg);
					}
				}
			}
		} else {
			m_status.release();
		}
		return 0;
	}

	service_ptr_t<ui_status_text_override> m_status;

	PFC_CLASS_NOT_COPYABLE(CMenuSelectionReceiver,CMenuSelectionReceiver);
};

class CMenuDescriptionMap : public CMenuSelectionReceiver {
public:
	CMenuDescriptionMap(HWND p_parent) : CMenuSelectionReceiver(p_parent) {}
	void Set(unsigned p_id,const char * p_description) {m_content.set(p_id,p_description);}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
};

class CMenuDescriptionHybrid : public CMenuSelectionReceiver {
public:
	CMenuDescriptionHybrid(HWND parent) : CMenuSelectionReceiver(parent) {}
	void Set(unsigned id, const char * desc) {m_content.set(id, desc);}

	void SetCM(contextmenu_manager::ptr mgr, unsigned base, unsigned max) {
		m_cmMgr = mgr; m_cmMgr_base = base; m_cmMgr_max = max;
	}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		if (m_cmMgr.is_valid() && p_id >= m_cmMgr_base && p_id < m_cmMgr_max) {
			return m_cmMgr->get_description_by_id(p_id - m_cmMgr_base,p_out);
		}
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
	contextmenu_manager::ptr m_cmMgr; unsigned m_cmMgr_base, m_cmMgr_max;
};

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CPoint & p_point) {
	return p_fmt << "(" << p_point.x << "," << p_point.y << ")";
}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CRect & p_rect) {
	return p_fmt << "(" << p_rect.left << "," << p_rect.top << "," << p_rect.right << "," << p_rect.bottom << ")";
}

//BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID)
#define END_MSG_MAP_HOOK() \
			break; \
		default: \
			return __super::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID); \
		} \
		return FALSE; \
	}


template<typename TClass>
class CAddDummyMessageMap : public TClass {
public:
	BEGIN_MSG_MAP(CAddDummyMessageMap<TClass>)
	END_MSG_MAP()
};

template<typename TClass>
class CWindowAutoLifetime : public TClass {
public:
	CWindowAutoLifetime(HWND parent) : TClass() {Init(parent);}
	template<typename TParam1> CWindowAutoLifetime(HWND parent, const TParam1 & p1) : TClass(p1) {Init(parent);}
	template<typename TParam1,typename TParam2> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2) : TClass(p1,p2) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3) : TClass(p1,p2,p3) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4) : TClass(p1,p2,p3,p4) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4,typename TParam5> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4, const TParam5 & p5) : TClass(p1,p2,p3,p4,p5) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4,typename TParam5,typename TParam6> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4, const TParam5 & p5, const TParam6 & p6) : TClass(p1,p2,p3,p4,p5,p6) {Init(parent);}
private:
	void Init(HWND parent) {WIN32_OP(this->Create(parent) != NULL);}
	void OnFinalMessage(HWND wnd) {PFC_ASSERT_NO_EXCEPTION( TClass::OnFinalMessage(wnd) ); PFC_ASSERT_NO_EXCEPTION(delete this);}
};

template<typename TClass>
class ImplementModelessTracking : public TClass {
public:
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(ImplementModelessTracking, TClass);
	
	BEGIN_MSG_MAP_EX(ImplementModelessTracking)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(TClass)
	END_MSG_MAP_HOOK()
private:
	BOOL OnInitDialog(CWindow, LPARAM) {m_modeless.Set( m_hWnd ); SetMsgHandled(FALSE); return FALSE; }
	void OnDestroy() {m_modeless.Set(NULL); SetMsgHandled(FALSE); }
	CModelessDialogEntry m_modeless;
};


#if _WIN32_WINNT >= 0x501
static void HeaderControl_SetSortIndicator(CHeaderCtrl header, int column, bool isUp) {
	const int total = header.GetItemCount();
	for(int walk = 0; walk < total; ++walk) {
		HDITEM item = {}; item.mask = HDI_FORMAT;
		if (header.GetItem(walk,&item)) {
			DWORD newFormat = item.fmt;
			newFormat &= ~( HDF_SORTUP | HDF_SORTDOWN );
			if (walk == column) {
				newFormat |= isUp ? HDF_SORTUP : HDF_SORTDOWN;
			}
			if (newFormat != item.fmt) {
				item.fmt = newFormat;
				header.SetItem(walk,&item);
			}
		}
	}
}
#endif

typedef CWinTraits<WS_POPUP,WS_EX_TRANSPARENT|WS_EX_LAYERED|WS_EX_TOPMOST|WS_EX_TOOLWINDOW> CFlashWindowTraits;

class CFlashWindow : public CWindowImpl<CFlashWindow,CWindow,CFlashWindowTraits> {
public:
	void Activate(CWindow parent) {
		ShowAbove(parent);
		m_tickCount = 0;
		SetTimer(KTimerID, 500);
	}
	void Deactivate() throw() {
		ShowWindow(SW_HIDE); KillTimer(KTimerID);
	}

	void ShowAbove(CWindow parent) {
		if (m_hWnd == NULL) {
			WIN32_OP( Create(NULL) != NULL );
		}
		CRect rect;
		WIN32_OP_D( parent.GetWindowRect(rect) );
		WIN32_OP_D( SetWindowPos(NULL,rect,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW) );
		m_parent = parent;
	}

	void CleanUp() throw() {
		if (m_hWnd != NULL) DestroyWindow();
	}

	BEGIN_MSG_MAP_EX(CFlashWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_TIMER(OnTimer)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	DECLARE_WND_CLASS_EX(TEXT("{2E124D52-131F-4004-A569-2316615BE63F}"),0,COLOR_HIGHLIGHT);
private:
	void OnDestroy() throw() {
		KillTimer(KTimerID);
	}
	enum {
		KTimerID = 0x47f42dd0
	};
	void OnTimer(WPARAM id) {
		if (id == KTimerID) {
			switch(++m_tickCount) {
				case 1:
					ShowWindow(SW_HIDE);
					break;
				case 2:
					ShowAbove(m_parent);
					break;
				case 3:
					ShowWindow(SW_HIDE);
					KillTimer(KTimerID);
					break;
			}
		}
	}
	LRESULT OnCreate(LPCREATESTRUCT) throw() {
		SetLayeredWindowAttributes(*this,0,128,LWA_ALPHA);
		return 0;
	}
	CWindow m_parent;
	t_uint32 m_tickCount;
};

class CImageListContainer : public CImageList {
public:
	CImageListContainer() {}
	~CImageListContainer() {Destroy();}
private:
	const CImageListContainer & operator=(const CImageListContainer&);
	CImageListContainer(const CImageListContainer&);
};




#define MSG_WM_TIMER_EX(timerId, func) \
	if (uMsg == WM_TIMER && (UINT_PTR)wParam == timerId) \
	{ \
		SetMsgHandled(TRUE); \
		func(); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

#define MESSAGE_HANDLER_SIMPLE(msg, func) \
	if(uMsg == msg) \
	{ \
		SetMsgHandled(TRUE); \
		func(); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}


template<typename TBase> class CContainedWindowSimpleT : public CContainedWindowT<TBase>, public CMessageMap {
public:
	CContainedWindowSimpleT() : CContainedWindowT<TBase>(this) {}
	BEGIN_MSG_MAP(CContainedWindowSimpleT)
	END_MSG_MAP()
};


//! Special service_impl_t replacement for service classes that also implement ATL/WTL windows.

template<typename t_base>
class window_service_impl_t : public t_base {
private:
	typedef window_service_impl_t<t_base> t_self;
public:
	BEGIN_MSG_MAP_EX(window_service_impl_t)
		MSG_WM_DESTROY(OnDestroyPassThru)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP_HOOK()

	int FB2KAPI service_release() throw() {
		int ret = --m_counter; 
		if (ret == 0) {
			if (!InterlockedExchange(&m_delayedDestroyInProgress,1)) {
				PFC_ASSERT_NO_EXCEPTION( service_impl_helper::release_object_delayed(this); );
			} else if (m_hWnd != NULL) {
				if (!m_destroyWindowInProgress) { // don't double-destroy in weird scenarios
					PFC_ASSERT_NO_EXCEPTION( this->DestroyWindow() );
				}
			} else {
				PFC_ASSERT_NO_EXCEPTION( delete this );
			}
		}
		return ret;
	}
	int FB2KAPI service_add_ref() throw() {return ++m_counter;}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(window_service_impl_t,t_base,{m_destroyWindowInProgress = false; m_delayedDestroyInProgress = 0; })
private:
	void OnDestroyPassThru() {
		SetMsgHandled(FALSE); m_destroyWindowInProgress = true;
	}
	void OnFinalMessage(HWND p_wnd) {
		t_base::OnFinalMessage(p_wnd);
		service_ptr_t<service_base> bump(this);
	}
	volatile bool m_destroyWindowInProgress;
	volatile LONG m_delayedDestroyInProgress;
	pfc::refcounter m_counter;
};


static void AppendMenuPopup(HMENU menu, UINT flags, CMenu & popup, const TCHAR * label) {
	PFC_ASSERT( flags & MF_POPUP );
	WIN32_OP( CMenuHandle(menu).AppendMenu(flags, popup, label) );
	popup.Detach();
}


class CPopupTooltipMessage {
public:
	CPopupTooltipMessage() : m_toolinfo() {}
	void Show(const TCHAR * message, CWindow wndParent) {
		if (message == NULL && m_tooltip.m_hWnd == NULL) return;
		if (m_tooltip.m_hWnd == NULL) {
			WIN32_OP_D( m_tooltip.Create( NULL , NULL, NULL, TTS_BALLOON | TTS_NOPREFIX | WS_POPUP) );
		}
		if (m_tooltip.m_hWnd == NULL) return;

		if (m_tooltip.GetToolCount() > 0) {
			m_tooltip.TrackActivate(&m_toolinfo,FALSE);
			m_tooltip.DelTool(&m_toolinfo);
		}

		if (message != NULL) {
			m_toolinfo.cbSize = sizeof(m_toolinfo);
			m_toolinfo.uFlags = TTF_TRACK|TTF_IDISHWND|TTF_ABSOLUTE|TTF_TRANSPARENT|TTF_CENTERTIP;
			m_toolinfo.hwnd = wndParent;
			m_toolinfo.uId = 0;
			m_toolinfo.lpszText = const_cast<TCHAR*>(message);
			m_toolinfo.hinst = core_api::get_my_instance();
			if (m_tooltip.AddTool(&m_toolinfo)) {
				CRect rect;
				WIN32_OP_D( wndParent.GetWindowRect(rect) );
				m_tooltip.TrackPosition(rect.CenterPoint().x,rect.bottom);
				m_tooltip.TrackActivate(&m_toolinfo,TRUE);
			}
		}
	}

	void CleanUp() {
		if (m_tooltip.m_hWnd != NULL) {
			m_tooltip.DestroyWindow();
		}
	}
private:
	CToolTipCtrl m_tooltip;
	TOOLINFO m_toolinfo;
};
