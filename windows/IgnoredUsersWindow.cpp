#include "stdafx.h"
#include "Colors.h"
#include "Fonts.h"
#include "IgnoredUsersWindow.h"
#include "LineDlg.h"
#include "ResourceLoader.h"
#include "WinUtil.h"
#include "../client/UserManager.h"
#include "../client/Wildcards.h"
#include "../client/Util.h"

LRESULT IgnoredUsersWindow::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ctrlIgnored.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL |
	                   LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | /*LVS_NOCOLUMNHEADER |*/ LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_IGNORELIST);
	ctrlIgnored.SetExtendedListViewStyle(WinUtil::getListViewExStyle(false));
	ctrlIgnored.SetImageList(g_favUserImage.getIconList(), LVSIL_SMALL);
	setListViewColors(ctrlIgnored);

	ctrlIgnored.InsertColumn(0, CTSTRING(IGNORE_LIST_TEXT), LVCFMT_LEFT, 180, 0);
	ctrlIgnored.InsertColumn(1, CTSTRING(IGNORE_LIST_TYPE), LVCFMT_LEFT, 80, 0);
	WinUtil::setExplorerTheme(ctrlIgnored);

	ctrlAdd.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_PUSHBUTTON, 0, IDC_IGNORE_ADD);
	ctrlAdd.SetWindowText(CTSTRING(ADD));
	ctrlAdd.SetFont(Fonts::g_systemFont);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_PUSHBUTTON, 0, IDC_IGNORE_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(Fonts::g_systemFont);
	ctrlRemove.EnableWindow(FALSE);

	ctrlClear.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_PUSHBUTTON, 0, IDC_IGNORE_CLEAR);
	ctrlClear.SetWindowText(CTSTRING(IGNORE_CLEAR));
	ctrlClear.SetFont(Fonts::g_systemFont);

	insertIgnoreList();
	return 0;
}

LRESULT IgnoredUsersWindow::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	updateLayout();
	return 0;
}

struct SortedListItem
{
	tstring text;
	UserManager::IgnoreType type;
};

void IgnoredUsersWindow::insertIgnoreList()
{
	vector<UserManager::IgnoreListItem> ignoreList;
	UserManager::getInstance()->getIgnoreList(ignoreList);
	vector<SortedListItem> sortedList;
	sortedList.resize(ignoreList.size());
	size_t index = 0;
	for (const auto& item : ignoreList)
	{
		sortedList[index].text = Text::toT(item.data);
		sortedList[index].type = item.type;
		index++;
	}
	std::sort(sortedList.begin(), sortedList.end(),
		[](SortedListItem& a, SortedListItem& b) { return Util::defaultSort(a.text, b.text, true) < 0; });
	int selectedItem = -1;
	for (size_t i = 0; i < sortedList.size(); i++)
	{
		int type = sortedList[i].type;
		ctrlIgnored.insert(i, sortedList[i].text, type == UserManager::IGNORE_NICK ? 0 : 6);
		ctrlIgnored.SetItemText(i, 1, type == UserManager::IGNORE_NICK ? CTSTRING(IGNORE_TYPE_NICK) : CTSTRING(IGNORE_TYPE_WILDCARD));
		if (sortedList[i].text == selectedIgnore)
			selectedItem = i;
	}
	if (selectedItem != -1)
		ctrlIgnored.SelectItem(selectedItem);
	updateButtons();
}

int IgnoredUsersWindow::getCount() const
{
	return ctrlIgnored.GetItemCount();
}

void IgnoredUsersWindow::updateButtons()
{
	ctrlRemove.EnableWindow(ctrlIgnored.GetNextItem(-1, LVNI_SELECTED) != -1);
	ctrlClear.EnableWindow(ctrlIgnored.GetItemCount() != 0);
}

LRESULT IgnoredUsersWindow::onIgnoreAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */)
{
	LineDlg dlg;
	dlg.icon = IconBitmaps::CHAT_PROHIBIT;
	dlg.limitText = 64;
	dlg.allowEmpty = false;
	dlg.description = TSTRING(ENTER_IGNORED_NICK);
	dlg.title = TSTRING(IGNORE_USER_BY_NAME);
	dlg.checkBox = true;
	dlg.checkBoxText = ResourceManager::USE_WILDCARD;
	dlg.validator = [](LineDlg& dlg, tstring& errorMsg) -> bool
	{
		UserManager::IgnoreListItem item;
		item.data = Text::fromT(dlg.line);
		if (dlg.checked)
		{
			std::regex tmp;
			if (!Wildcards::regexFromPatternList(tmp, item.data, false))
			{
				errorMsg = TSTRING(INVALID_WILDCARD);
				return false;
			}
			item.type = UserManager::IGNORE_WILDCARD;
		}
		else
			item.type = UserManager::IGNORE_NICK;
		if (!UserManager::getInstance()->addToIgnoreList(item))
		{
			errorMsg = TSTRING(ALREADY_IGNORED);
			return false;
		}
		return true;
	};
	if (dlg.DoModal(m_hWnd) == IDOK)
		selectedIgnore = std::move(dlg.line);
	return 0;
}

LRESULT IgnoredUsersWindow::onIgnoreRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */)
{
	vector<string> userNames;
	int i = -1;
	while ((i = ctrlIgnored.GetNextItem(i, LVNI_SELECTED)) != -1)
		userNames.push_back(ctrlIgnored.ExGetItemText(i, 0));
	if (!userNames.empty())
		UserManager::getInstance()->removeFromIgnoreList(userNames);
	return 0;
}

LRESULT IgnoredUsersWindow::onIgnoreClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */)
{
	if (MessageBox(CTSTRING(CLEAR_LIST_OF_IGNORED_USERS), getAppNameVerT().c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		UserManager::getInstance()->clearIgnoreList();
	}
	return 0;
}

LRESULT IgnoredUsersWindow::onIgnoredItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	updateButtons();
	return 0;
}

void IgnoredUsersWindow::updateUsers()
{
	ctrlIgnored.DeleteAllItems();
	insertIgnoreList();
}

static int getButtonWidth(HWND hwnd, HDC dc, int padding, int minWidth)
{
	tstring text;
	WinUtil::getWindowText(hwnd, text);
	return std::max<int>(WinUtil::getTextWidth(text, dc) + padding, minWidth);
}

void IgnoredUsersWindow::initMetrics()
{
	WinUtil::getDialogUnits(m_hWnd, Fonts::g_systemFont, xdu, ydu);
	buttonHeight = WinUtil::dialogUnitsToPixelsY(14, ydu);
	buttonSpace = WinUtil::dialogUnitsToPixelsX(2, xdu);
	buttonSpaceLarge = WinUtil::dialogUnitsToPixelsX(10, xdu);
	buttonVertSpace = WinUtil::dialogUnitsToPixelsY(2, ydu);
	int buttonPadding = WinUtil::dialogUnitsToPixelsX(4, xdu);
	int buttonMinWidth = WinUtil::dialogUnitsToPixelsX(40, xdu);
	HDC dc = GetDC();
	buttonWidth[0] = getButtonWidth(ctrlAdd, dc, buttonPadding, buttonMinWidth);
	buttonWidth[1] = getButtonWidth(ctrlRemove, dc, buttonPadding, buttonMinWidth);
	buttonWidth[2] = getButtonWidth(ctrlClear, dc, buttonPadding, buttonMinWidth);
	ReleaseDC(dc);
}

void IgnoredUsersWindow::updateLayout()
{
	CRect rect;
	GetClientRect(&rect);

	if (!xdu) initMetrics();

	CRect rcList = rect;
	rcList.bottom -= buttonHeight + buttonVertSpace;
	HDWP dwp = BeginDeferWindowPos(4);
	ctrlIgnored.DeferWindowPos(dwp, nullptr, rcList.left, rcList.top, rcList.Width(), rcList.Height(), SWP_NOZORDER);

	CRect rcControl;
	rcControl.top = rcList.bottom + buttonVertSpace;
	rcControl.bottom = rcControl.top + buttonHeight;
	rcControl.left = rect.left;
	rcControl.right = rcControl.left + buttonWidth[0];
	ctrlAdd.DeferWindowPos(dwp, nullptr, rcControl.left, rcControl.top, rcControl.Width(), rcControl.Height(), SWP_NOZORDER);

	rcControl.left = rcControl.right + buttonSpace;
	rcControl.right = rcControl.left + buttonWidth[1];
	ctrlRemove.DeferWindowPos(dwp, nullptr, rcControl.left, rcControl.top, rcControl.Width(), rcControl.Height(), SWP_NOZORDER);

	rcControl.right = rect.right - buttonSpace;
	rcControl.left = rcControl.right - buttonWidth[2];
	ctrlClear.DeferWindowPos(dwp, nullptr, rcControl.left, rcControl.top, rcControl.Width(), rcControl.Height(), SWP_NOZORDER);
	EndDeferWindowPos(dwp);
}

int IgnoredUsersWindow::getMinWidth()
{
	dcassert(m_hWnd);
	if (!xdu) initMetrics();
	return buttonWidth[0] + buttonWidth[1] + buttonWidth[2] + 2*buttonSpace + buttonSpaceLarge;
}

void IgnoredUsersWindow::updateColors()
{
	setListViewColors(ctrlIgnored);
}
