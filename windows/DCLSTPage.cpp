/*
 * Copyright (C) 2011-2017 FlylinkDC++ Team http://flylinkdc.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "DCLSTPage.h"
#include "WinUtil.h"
#include "BrowseFile.h"
#include "ConfUI.h"
#include "../client/SettingsManager.h"
#include "../client/PathUtil.h"

static const WinUtil::TextItem texts[] =
{
	{ IDC_DCLS_GENERATORBORDER, ResourceManager::DCLS_GENERATORBORDER },
	{ IDC_DCLS_CREATE_IN_FOLDER, ResourceManager::DCLS_CREATE_IN_FOLDER },
	{ IDC_DCLS_ANOTHER_FOLDER, ResourceManager::DCLS_ANOTHER_FOLDER },
	{ IDC_DCLST_CLICK_STATIC, ResourceManager::DCLS_CLICK_ACTION },
	{ IDC_DCLST_INCLUDESELF, ResourceManager::DCLST_INCLUDESELF },
	{ 0, ResourceManager::Strings() }
};

static const PropPage::Item items[] =
{
	{ IDC_DCLS_FOLDER, Conf::DCLST_DIRECTORY, PropPage::T_STR },
	{ IDC_DCLST_INCLUDESELF, Conf::DCLST_INCLUDESELF, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT DCLSTPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	WinUtil::translate(*this, texts);
	PropPage::read(*this, items);

	const auto* ss = SettingsManager::instance.getUiSettings();
	CheckDlgButton(ss->getBool(Conf::DCLST_CREATE_IN_SAME_FOLDER) ?
		IDC_DCLS_CREATE_IN_FOLDER : IDC_DCLS_ANOTHER_FOLDER, BST_CHECKED);

	magnetClick.Attach(GetDlgItem(IDC_DCLST_CLICK));
	magnetClick.AddString(CTSTRING(ASK));
	magnetClick.AddString(CTSTRING(MAGNET_DLG_BRIEF_SEARCH));
	magnetClick.AddString(CTSTRING(MAGNET_DLG_BRIEF_DL_DCLST));
	magnetClick.AddString(CTSTRING(MAGNET_DLG_BRIEF_SHOW_DCLST));
	
	if (ss->getBool(Conf::DCLST_ASK))
	{
		magnetClick.SetCurSel(0);
	}
	else
	{
		int action = ss->getInt(Conf::DCLST_ACTION);
		magnetClick.SetCurSel(action + 1);
	}
	
	fixControls();
	return TRUE;
}

void DCLSTPage::write()
{
	PropPage::write(*this, items);

	auto ss = SettingsManager::instance.getUiSettings();
	ss->setBool(Conf::DCLST_CREATE_IN_SAME_FOLDER, IsDlgButtonChecked(IDC_DCLS_CREATE_IN_FOLDER) == BST_CHECKED);
	int index = magnetClick.GetCurSel();
	if (index == 0)
	{
		ss->setBool(Conf::DCLST_ASK, true);
	}
	else
	{
		ss->setBool(Conf::DCLST_ASK, false);
		ss->setInt(Conf::DCLST_ACTION, index - 1);
	}
}

LRESULT DCLSTPage::onClickedDCLSTFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	fixControls();
	return 0;
}

void DCLSTPage::fixControls()
{
	BOOL enable = IsDlgButtonChecked(IDC_DCLS_ANOTHER_FOLDER) == BST_CHECKED;
	GetDlgItem(IDC_DCLS_FOLDER).EnableWindow(enable);
	GetDlgItem(IDC_PREVIEW_BROWSE).EnableWindow(enable);
}

LRESULT DCLSTPage::onBrowseClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target;
	if (WinUtil::browseDirectory(target, m_hWnd))
	{
		CWindow ctrlName = GetDlgItem(IDC_DCLS_FOLDER);
		ctrlName.SetWindowText(target.c_str());
		if (ctrlName.GetWindowTextLength() == 0)
			ctrlName.SetWindowText(Util::getLastDir(target).c_str());
	}
	return 0;
}
