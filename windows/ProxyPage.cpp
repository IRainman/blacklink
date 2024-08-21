/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#include "Resource.h"
#include "ProxyPage.h"
#include "WinUtil.h"
#include "DialogLayout.h"
#include "../client/SettingsManager.h"
#include "../client/Socket.h"
#include "../client/ConfCore.h"

using DialogLayout::FLAG_TRANSLATE;
using DialogLayout::UNSPEC;
using DialogLayout::AUTO;

static const DialogLayout::Item layoutItems[] =
{
	{ IDC_SETTINGS_OUTGOING, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_DIRECT_OUT, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_SOCKS5, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_SETTINGS_SOCKS5_IP, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_SETTINGS_SOCKS5_IP, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_SETTINGS_SOCKS5_PORT, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_SETTINGS_SOCKS5_USERNAME, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_SOCKS_RESOLVE, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_USE_HTTP_PROXY, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_CAPTION_HTTP_PROXY_URL, FLAG_TRANSLATE, UNSPEC, UNSPEC }
};

static const PropPage::Item items[] =
{
	{ IDC_SOCKS_SERVER,   Conf::SOCKS_SERVER,   PropPage::T_STR  },
	{ IDC_SOCKS_PORT,     Conf::SOCKS_PORT,     PropPage::T_INT  },
	{ IDC_SOCKS_USER,     Conf::SOCKS_USER,     PropPage::T_STR  },
	{ IDC_SOCKS_PASSWORD, Conf::SOCKS_PASSWORD, PropPage::T_STR  },
	{ IDC_SOCKS_RESOLVE,  Conf::SOCKS_RESOLVE,  PropPage::T_BOOL },
	{ IDC_USE_HTTP_PROXY, Conf::USE_HTTP_PROXY, PropPage::T_BOOL },
	{ IDC_HTTP_PROXY_URL, Conf::HTTP_PROXY,     PropPage::T_STR  },
	{ 0,                  0,                    PropPage::T_END  }
};

LRESULT ProxyPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DialogLayout::layout(m_hWnd, layoutItems, _countof(layoutItems));

	auto ss = SettingsManager::instance.getCoreSettings();
	ss->lockRead();
	int outgoingConnections = ss->getInt(Conf::OUTGOING_CONNECTIONS);
	ss->unlockRead();
	switch (outgoingConnections)
	{
		case Conf::OUTGOING_SOCKS5:
			CheckDlgButton(IDC_SOCKS5, BST_CHECKED);
			break;
		default:
			CheckDlgButton(IDC_DIRECT_OUT, BST_CHECKED);
			break;
	}

	PropPage::read(*this, items);

	fixControls();

	CEdit(GetDlgItem(IDC_SOCKS_SERVER)).LimitText(250);
	CEdit(GetDlgItem(IDC_SOCKS_PORT)).LimitText(5);
	CEdit(GetDlgItem(IDC_SOCKS_USER)).LimitText(250);
	CEdit(GetDlgItem(IDC_SOCKS_PASSWORD)).LimitText(250);

	return TRUE;
}

void ProxyPage::write()
{
	tstring x;
	WinUtil::getWindowText(GetDlgItem(IDC_SOCKS_SERVER), x);
	tstring::size_type i;

	while ((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	SetDlgItemText(IDC_SOCKS_SERVER, x.c_str());

	WinUtil::getWindowText(GetDlgItem(IDC_SERVER), x);

	while ((i = x.find(' ')) != string::npos)
		x.erase(i, 1);

	SetDlgItemText(IDC_SERVER, x.c_str());

	PropPage::write(*this, items);

	int ct = Conf::OUTGOING_DIRECT;
	if (IsDlgButtonChecked(IDC_SOCKS5))
		ct = Conf::OUTGOING_SOCKS5;

	auto ss = SettingsManager::instance.getCoreSettings();
	ss->lockWrite();
	ss->setInt(Conf::OUTGOING_CONNECTIONS, ct);
	ss->unlockWrite();
}

void ProxyPage::fixControls()
{
	BOOL socks = IsDlgButtonChecked(IDC_SOCKS5);
	GetDlgItem(IDC_SOCKS_SERVER).EnableWindow(socks);
	GetDlgItem(IDC_SOCKS_PORT).EnableWindow(socks);
	GetDlgItem(IDC_SOCKS_USER).EnableWindow(socks);
	GetDlgItem(IDC_SOCKS_PASSWORD).EnableWindow(socks);
	GetDlgItem(IDC_SOCKS_RESOLVE).EnableWindow(socks);
	BOOL httpProxy = IsDlgButtonChecked(IDC_USE_HTTP_PROXY);
	GetDlgItem(IDC_HTTP_PROXY_URL).EnableWindow(httpProxy);
}

LRESULT ProxyPage::onClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	fixControls();
	return 0;
}
