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
#include "WinUtil.h"
#include "FavHubProperties.h"
#include "KnownClients.h"
#include "DialogLayout.h"
#include "../client/SettingsManager.h"
#include "../client/FavoriteManager.h"
#include "../client/Random.h"
#include "../client/Util.h"
#include "../client/ConfCore.h"
#include <boost/algorithm/string/trim.hpp>

using DialogLayout::FLAG_TRANSLATE;
using DialogLayout::UNSPEC;
using DialogLayout::AUTO;

static const WinUtil::TextItem textsIdent[] =
{
	{ IDC_CAPTION_BLANK,            ResourceManager::LEAVE_BLANK_FOR_DEFAULTS        },
	{ IDC_FH_NICK,                  ResourceManager::NICK                            },
	{ IDC_WIZARD_NICK_RND,          ResourceManager::WIZARD_NICK_RND                 },
	{ IDC_FH_PASSWORD,              ResourceManager::PASSWORD                        },
	{ IDC_FH_USER_DESC,             ResourceManager::DESCRIPTION                     },
	{ IDC_FH_EMAIL,                 ResourceManager::EMAIL                           },
	{ IDC_FH_SHARE_GROUP,           ResourceManager::SHARE_GROUP                     },
	{ IDC_FH_AWAY,                  ResourceManager::AWAY_MESSAGE                    },
	{ IDC_CLIENT_ID,                ResourceManager::CLIENT_ID                       },
	{ 0,                            ResourceManager::Strings()                       }
};

static const WinUtil::TextItem textsOptions[] =
{
	{ IDC_CAPTION_ENCODING,         ResourceManager::FAVORITE_HUB_CHARACTER_SET      },
	{ IDC_CAPTION_CONNECTION_TYPE,  ResourceManager::CONNECTION_TYPE                 },
	{ IDC_SETTINGS_IP,              ResourceManager::IP_ADDRESS                      },
	{ IDC_EXCL_CHECKS,              ResourceManager::EXCL_CHECKS                     },
	{ IDC_SHOW_JOINS,               ResourceManager::SHOW_JOINS                      },
	{ IDC_SUPPRESS_FAV_CHAT_AND_PM, ResourceManager::SUPPRESS_FAV_CHAT_AND_PM        },
	{ IDC_CAPTION_SEARCH_INTERVAL,  ResourceManager::MINIMUM_SEARCH_INTERVAL         },
	{ IDC_OVERRIDE_DEFAULT,         ResourceManager::OVERRIDE_DEFAULT_VALUES         },
	{ IDC_CAPTION_ACTIVE,           ResourceManager::SEARCH_INTERVAL_ACTIVE          },
	{ IDC_S,                        ResourceManager::S                               },
	{ IDC_CAPTION_PASSIVE,          ResourceManager::SEARCH_INTERVAL_PASSIVE         },
	{ IDC_S_PASSIVE,                ResourceManager::S                               },
	{ 0,                            ResourceManager::Strings()                       }
};

static const WinUtil::TextItem textsAdvanced[] =
{
	{ IDC_CAPTION_OPCHAT,           ResourceManager::OPCHAT                          },
	{ 0,                            ResourceManager::Strings()                       }
};

static const WinUtil::TextItem texts[] =
{
	{ IDOK,                         ResourceManager::OK                              },
	{ IDCANCEL,                     ResourceManager::CANCEL                          },
	{ 0,                            ResourceManager::Strings()                       }
};

static const DialogLayout::Align align4 = { -1, DialogLayout::SIDE_RIGHT, U_DU(4) };
static const DialogLayout::Align align5 = { 5,  DialogLayout::SIDE_RIGHT, U_DU(4) };
static const DialogLayout::Align align6 = { -1, DialogLayout::SIDE_LEFT,  0 };
static const DialogLayout::Align align7 = { -1, DialogLayout::SIDE_RIGHT, 0 };

static const DialogLayout::Item layoutItemsName[] =
{
	{ IDC_FH_NAME, FLAG_TRANSLATE, AUTO, UNSPEC, 1 },
	{ IDC_FH_ADDRESS, FLAG_TRANSLATE, AUTO, UNSPEC, 1 },
	{ IDC_FH_HUB_DESC, FLAG_TRANSLATE, AUTO, UNSPEC, 1 },
	{ IDC_HUBNAME, 0, UNSPEC, UNSPEC, 0, &align4 },
	{ IDC_HUBADDR, 0, UNSPEC, UNSPEC, 0, &align4 },
	{ IDC_HUBDESCR, 0, UNSPEC, UNSPEC, 0, &align4 },
	{ IDC_CAPTION_KEYPRINT, FLAG_TRANSLATE, AUTO, UNSPEC, 0, &align4 },
	{ IDC_FAVGROUP, FLAG_TRANSLATE, AUTO, UNSPEC, 0, &align4 },
	{ IDC_KEYPRINT, 0, UNSPEC, UNSPEC, 0, &align4 },
	{ IDC_FAVGROUP_BOX, 0, UNSPEC, UNSPEC, 0, &align4 },
	{ IDC_PREFER_IPV6, FLAG_TRANSLATE, AUTO, UNSPEC, 0, &align5 },
	{ IDC_FH_NAME, 0, UNSPEC, UNSPEC, 0, &align6, &align7 },
	{ IDC_FH_ADDRESS, 0, UNSPEC, UNSPEC, 0, &align6, &align7 },
	{ IDC_FH_HUB_DESC, 0, UNSPEC, UNSPEC, 0, &align6, &align7 }
};

static const DialogLayout::Align align1 = { 4, DialogLayout::SIDE_RIGHT, U_DU(4) };
static const DialogLayout::Align align2 = { 3, DialogLayout::SIDE_RIGHT, 0 };
static const DialogLayout::Align align3 = { 5, DialogLayout::SIDE_RIGHT, U_DU(2) };

static const DialogLayout::Item layoutItemsCheats[] =
{
	{ IDC_EXCLUSIVE_HUB, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_FAKE_SHARE, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_SIZE_TYPE, 0, UNSPEC, UNSPEC },
	{ IDC_CAPTION_FILE_COUNT, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_FILE_COUNT, 0, UNSPEC, UNSPEC, 0, &align1, &align2 },
	{ IDC_WIZARD_NICK_RND, FLAG_TRANSLATE, UNSPEC, UNSPEC, 0, &align3 },
	{ IDC_FAKE_SLOTS, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_OVERRIDE_STATUS, FLAG_TRANSLATE, AUTO, UNSPEC }
};

static void filterText(HWND hwnd, const TCHAR* filter)
{
	CEdit edit(hwnd);
	tstring buf;
	WinUtil::getWindowText(edit, buf);
	bool changed = false;

	if (!buf.empty())
	{
		tstring::size_type i = 0;
		while ((i = buf.find_first_of(filter, i)) != tstring::npos)
		{
			buf.erase(i, 1);
			changed = true;
		}
	}
	if (changed)
	{
		int start, end;
		edit.GetSel(start, end);
		edit.SetWindowText(buf.c_str());
		if (start > 0) start--;
		if (end > 0) end--;
		edit.SetSel(start, end);
	}
}

#define ADD_TAB(name, type, text) \
	tcItem.pszText = const_cast<TCHAR*>(CTSTRING(text)); \
	tcItem.lParam = reinterpret_cast<LPARAM>(&name); \
	name.Create(m_hWnd, type::IDD); \
	ctrlTabs.InsertItem(n++, &tcItem);

LRESULT FavHubProperties::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ctrlTabs.Attach(GetDlgItem(IDC_TABS));

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT | TCIF_PARAM;
	tcItem.iImage = -1;

	int n = 0;
	ADD_TAB(tabName, FavoriteHubTabName, FAV_HUB_NAME);
	ADD_TAB(tabIdent, FavoriteHubTabIdent, FAV_HUB_IDENT);
	ADD_TAB(tabOptions, FavoriteHubTabOptions, FAV_HUB_OPTIONS);
	ADD_TAB(tabCheats, FavoriteHubTabCheats, FAV_HUB_CHEATS);
	ADD_TAB(tabAdvanced, FavoriteHubTabAdvanced, FAV_HUB_ADVANCED);

	ctrlTabs.SetCurSel(0);
	changeTab();

	SetWindowText(CTSTRING(FAVORITE_HUB_PROPERTIES));
	WinUtil::translate(*this, texts);

	HICON dialogIcon = g_iconBitmaps.getIcon(IconBitmaps::FAVORITES, 0);
	SetIcon(dialogIcon, FALSE);
	SetIcon(dialogIcon, TRUE);

	CenterWindow(GetParent());

	return FALSE;
}

void FavHubProperties::changeTab()
{
	int pos = ctrlTabs.GetCurSel();
	tabName.ShowWindow(SW_HIDE);
	tabIdent.ShowWindow(SW_HIDE);
	tabOptions.ShowWindow(SW_HIDE);
	tabCheats.ShowWindow(SW_HIDE);
	tabAdvanced.ShowWindow(SW_HIDE);

	CRect rc;
	ctrlTabs.GetClientRect(&rc);
	ctrlTabs.AdjustRect(FALSE, &rc);
	ctrlTabs.MapWindowPoints(m_hWnd, &rc);

	switch (pos)
	{
		case 0:
			tabName.MoveWindow(&rc);
			tabName.ShowWindow(SW_SHOW);
			break;

		case 1:
			tabIdent.MoveWindow(&rc);
			tabIdent.ShowWindow(SW_SHOW);
			break;

		case 2:
			tabOptions.MoveWindow(&rc);
			tabOptions.ShowWindow(SW_SHOW);
			break;

		case 3:
			tabCheats.MoveWindow(&rc);
			tabCheats.ShowWindow(SW_SHOW);
			break;

		case 4:
			tabAdvanced.MoveWindow(&rc);
			tabAdvanced.ShowWindow(SW_SHOW);
			break;
	}
}

LRESULT FavHubProperties::onClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK)
	{
		tstring buf;
		WinUtil::getWindowText(tabName.ctrlAddress, buf);
		if (buf.empty())
		{
			MessageBox(CTSTRING(INCOMPLETE_FAV_HUB), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
			return 0;
		}

		string url = Text::fromT(buf);
		Util::ParsedUrl p;
		Util::decodeUrl(url, p);
		if (!Util::getHubProtocol(p.protocol))
		{
			MessageBox(CTSTRING_F(UNSUPPORTED_HUB_PROTOCOL, Text::toT(p.protocol)), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
			return 0;
		}

		if (p.host.empty())
		{
			MessageBox(CTSTRING(INCOMPLETE_FAV_HUB), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
			return 0;
		}

		WinUtil::getWindowText(tabName.ctrlKeyPrint, buf);
		string keyPrint = Text::fromT(buf);
		boost::trim(keyPrint);
		if (keyPrint.empty())
		{
			keyPrint = Util::getQueryParam(p.query, "kp");
			boost::trim(keyPrint);
		}
		if (!keyPrint.empty() && !HubEntry::checkKeyPrintFormat(keyPrint))
		{
			MessageBox(CTSTRING(INVALID_KEYPRINT), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
			return 0;
		}

		url = Util::formatDchubUrl(p);

		if (tabName.addressChanged && FavoriteManager::getInstance()->isFavoriteHub(url, entry->getID()))
		{
			MessageBox(CTSTRING(FAVORITE_HUB_ALREADY_EXISTS), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
			return 0;
		}

		tstring fakeShare;
		if (tabCheats.ctrlEnableFakeShare.GetCheck() == BST_CHECKED)
		{
			static const TCHAR units[] = _T("KMGT");
			WinUtil::getWindowText(tabCheats.ctrlFakeShare, fakeShare);
			double unused;
			if (!Util::toDouble(unused, Text::fromT(fakeShare)))
			{
				MessageBox(CTSTRING(FAV_BAD_SIZE), getAppNameVerT().c_str(), MB_ICONWARNING | MB_OK);
				return 0;
			}
			int unit = tabCheats.ctrlFakeShareUnit.GetCurSel();
			if (unit > 0 && unit < 5) fakeShare += units[unit-1];
		}

		entry->setServer(url);
		entry->setKeyPrint(keyPrint);
		entry->setPreferIP6(tabName.ctrlPreferIP6.GetCheck() == BST_CHECKED);

		WinUtil::getWindowText(tabName.ctrlName, buf);
		string name = Text::fromT(buf);
		if (name.empty()) name = p.host;
		entry->setName(name);

		WinUtil::getWindowText(tabName.ctrlDesc, buf);
		entry->setDescription(Text::fromT(buf));

		WinUtil::getWindowText(tabIdent.ctrlNick, buf);
		entry->setNick(Text::fromT(buf));

		WinUtil::getWindowText(tabIdent.ctrlPassword, buf);
		entry->setPassword(Text::fromT(buf));

		WinUtil::getWindowText(tabIdent.ctrlDesc, buf);
		entry->setUserDescription(Text::fromT(buf));

		WinUtil::getWindowText(tabIdent.ctrlAwayMsg, buf);
		entry->setAwayMsg(Text::fromT(buf));

		WinUtil::getWindowText(tabIdent.ctrlEmail, buf);
		entry->setEmail(Text::fromT(buf));

		int shareGroupIndex = tabIdent.ctrlShareGroup.GetCurSel();
		const auto& sg = tabIdent.shareGroups.v[shareGroupIndex];
		entry->setShareGroup(sg.id);
		entry->setHideShare(sg.def == ShareGroupList::HIDE_SHARE_DEF);

		entry->setShowJoins(tabOptions.ctrlShowJoins.GetCheck() == BST_CHECKED);
		entry->setExclChecks(tabOptions.ctrlExclChecks.GetCheck() == BST_CHECKED);
		entry->setSuppressChatAndPM(tabOptions.ctrlSuppressMsg.GetCheck() == BST_CHECKED);

		entry->setExclusiveHub(tabCheats.ctrlFakeHubCount.GetCheck() == BST_CHECKED);

		WinUtil::getWindowText(tabOptions.ctrlIpAddress, buf);
		entry->setIP(Text::fromT(buf));

		WinUtil::getWindowText(tabAdvanced.ctrlOpChat, buf);
		entry->setOpChat(Text::fromT(buf));

		if (tabOptions.ctrlSearchOverride.GetCheck() == BST_CHECKED)
		{
			WinUtil::getWindowText(tabOptions.ctrlSearchActive, buf);
			entry->setSearchInterval(Util::toUInt32(buf));

			WinUtil::getWindowText(tabOptions.ctrlSearchPassive, buf);
			entry->setSearchIntervalPassive(Util::toUInt32(buf));
		}
		else
		{
			entry->setSearchInterval(0);
			entry->setSearchIntervalPassive(0);
		}

		if (tabName.ctrlGroup.GetCurSel() == 0)
		{
			entry->setGroup(Util::emptyString);
		}
		else
		{
			WinUtil::getWindowText(tabName.ctrlGroup, buf);
			entry->setGroup(Text::fromT(buf));
		}

		WinUtil::getWindowText(tabIdent.ctrlClientId, buf);
		string clientName, clientVersion;
		FavoriteManager::splitClientId(Text::fromT(buf), clientName, clientVersion);
		entry->setClientName(clientName);
		entry->setClientVersion(clientVersion);
		entry->setOverrideId(tabIdent.IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED);

		entry->setFakeShare(Text::fromT(fakeShare));

		int64_t fakeCount = -1;
		if (!fakeShare.empty())
		{
			WinUtil::getWindowText(tabCheats.ctrlFakeCount, buf);
			fakeCount = Util::toInt64(buf);
			if (fakeCount <= 0) fakeCount = -1;
		}
		entry->setFakeFileCount(fakeCount);

		int fakeSlots = -1;
		if (tabCheats.ctrlEnableFakeSlots.GetCheck() == BST_CHECKED)
		{
			WinUtil::getWindowText(tabCheats.ctrlFakeSlots, buf);
			if (!buf.empty()) fakeSlots = Util::toInt(buf);
		}
		entry->setFakeSlots(fakeSlots);

		int fakeClientStatus = 0;
		if (tabCheats.ctrlOverrideStatus.GetCheck() == BST_CHECKED)
			fakeClientStatus = tabCheats.ctrlFakeStatus.GetCurSel() + 1;
		entry->setFakeClientStatus(fakeClientStatus);

		entry->setMode(tabOptions.ctrlConnType.GetCurSel());

		if (Util::isAdcHub(entry->getServer()))
			entry->setEncoding(Text::CHARSET_UTF8);
		else
			entry->setEncoding(WinUtil::getSelectedCharset(tabOptions.ctrlEncoding));
	}
	EndDialog(wID);
	return 0;
}

LRESULT FavoriteHubTabName::onInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
	DialogLayout::layout(m_hWnd, layoutItemsName, _countof(layoutItemsName));

	ctrlName.Attach(GetDlgItem(IDC_HUBNAME));
	ctrlName.SetWindowText(Text::toT(entry->getName()).c_str());

	ctrlDesc.Attach(GetDlgItem(IDC_HUBDESCR));
	ctrlDesc.SetWindowText(Text::toT(entry->getDescription()).c_str());

	ctrlAddress.Attach(GetDlgItem(IDC_HUBADDR));
	ctrlAddress.SetWindowText(Text::toT(entry->getServer()).c_str());

	ctrlPreferIP6.Attach(GetDlgItem(IDC_PREFER_IPV6));
	ctrlPreferIP6.SetCheck(entry->getPreferIP6() ? BST_CHECKED : BST_UNCHECKED);

	ctrlKeyPrint.Attach(GetDlgItem(IDC_KEYPRINT));
	ctrlKeyPrint.SetWindowText(Text::toT(entry->getKeyPrint()).c_str());

	ctrlGroup.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
	ctrlGroup.AddString(_T("---"));
	int selIndex = 0;

	{
		FavoriteManager::LockInstanceHubs lock(FavoriteManager::getInstance(), false);
		const FavHubGroups& favHubGroups = lock.getFavHubGroups();
		for (auto i = favHubGroups.cbegin(); i != favHubGroups.cend(); ++i)
		{
			const string& name = i->first;
			int pos = ctrlGroup.AddString(Text::toT(name).c_str());

			if (name == entry->getGroup())
				selIndex = pos;
		}
	}
	ctrlGroup.SetCurSel(selIndex);

	ctrlName.SetFocus();
	ctrlName.SetSel(0, -1);
	addressChanged = false;

	return FALSE;
}

LRESULT FavoriteHubTabName::onTextChanged(WORD, WORD wID, HWND hWndCtl, BOOL&)
{
	addressChanged = true;
	filterText(hWndCtl, _T("$|<> '\"@#\\"));
	return TRUE;
}

LRESULT FavoriteHubTabIdent::onInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
	WinUtil::translate(*this, textsIdent);

	ctrlNick.Attach(GetDlgItem(IDC_HUBNICK));
	ctrlNick.SetWindowText(Text::toT(entry->getNick(false)).c_str());

	ctrlPassword.Attach(GetDlgItem(IDC_HUBPASS));
	ctrlPassword.SetWindowText(Text::toT(entry->getPassword()).c_str());

	ctrlDesc.Attach(GetDlgItem(IDC_HUBUSERDESCR));
	ctrlDesc.SetWindowText(Text::toT(entry->getUserDescription()).c_str());

	ctrlEmail.Attach(GetDlgItem(IDC_HUBEMAIL));
	ctrlEmail.SetWindowText(Text::toT(entry->getEmail()).c_str());

	ctrlAwayMsg.Attach(GetDlgItem(IDC_HUBAWAY));
	ctrlAwayMsg.SetWindowText(Text::toT(entry->getAwayMsg()).c_str());

	ctrlShareGroup.Attach(GetDlgItem(IDC_SHARE_GROUP));
	shareGroups.init();
	shareGroups.fillCombo(ctrlShareGroup, entry->getShareGroup(), entry->getHideShare());

	ctrlClientId.Attach(GetDlgItem(IDC_CLIENT_ID_BOX));
	for (size_t i = 0; KnownClients::clients[i].name; ++i)
	{
		string clientId = KnownClients::clients[i].name;
		clientId += ' ';
		clientId += KnownClients::clients[i].version;
		ctrlClientId.AddString(Text::toT(clientId).c_str());
	}
	if (!entry->getClientName().empty())
	{
		string clientId = entry->getClientName() + ' ' + entry->getClientVersion();
		ctrlClientId.SetWindowText(Text::toT(clientId).c_str());
	}

	CheckDlgButton(IDC_CLIENT_ID, entry->getOverrideId() ? BST_CHECKED : BST_UNCHECKED);
	ctrlClientId.EnableWindow(entry->getOverrideId() ? TRUE : FALSE);

	ctrlNick.LimitText(35);
	ctrlPassword.LimitText(64);
	ctrlDesc.LimitText(50);

	return FALSE;
}

LRESULT FavoriteHubTabIdent::onRandomNick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ctrlNick.SetWindowText(Text::toT(Util::getRandomNick()).c_str());
	return 0;
}

LRESULT FavoriteHubTabIdent::onChangeClientId(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ctrlClientId.EnableWindow(IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED);
	return 0;
}

LRESULT FavoriteHubTabIdent::onTextChanged(WORD, WORD wID, HWND hWndCtl, BOOL&)
{
	const TCHAR* filter;
	switch (wID)
	{
		case IDC_HUBPASS:
			filter = _T("$|");
			break;
		case IDC_HUBUSERDESCR:
			filter = _T("$|<>");
			break;
		default: // IDC_HUBNICK, IDC_HUBEMAIL
			filter = _T("$|<> ");
	}
	filterText(hWndCtl, filter);
	return TRUE;
}

LRESULT FavoriteHubTabOptions::onInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
	WinUtil::translate(*this, textsOptions);

	ctrlEncoding.Attach(GetDlgItem(IDC_ENCODING));
	if (Util::isAdcHub(entry->getServer()))
	{
		// select UTF-8 for ADC hubs
		WinUtil::fillCharsetList(ctrlEncoding, 0, true, true);
		ctrlEncoding.EnableWindow(FALSE);
	}
	else
		WinUtil::fillCharsetList(ctrlEncoding, entry->getEncoding(), false, true);

	static const ResourceManager::Strings connTypeStrings[] =
	{
		R_(DEFAULT), R_(SETTINGS_DIRECT), R_(SETTINGS_FIREWALL_PASSIVE), R_INVALID
	};
	ctrlConnType.Attach(GetDlgItem(IDC_CONNECTION_TYPE));
	WinUtil::fillComboBoxStrings(ctrlConnType, connTypeStrings);

	int selIndex = entry->getMode();
	if (selIndex != 1 && selIndex != 2) selIndex = 0;
	ctrlConnType.SetCurSel(selIndex);

	ctrlIpAddress.Attach(GetDlgItem(IDC_SERVER));
	ctrlIpAddress.SetWindowText(Text::toT(entry->getIP()).c_str());

	ctrlExclChecks.Attach(GetDlgItem(IDC_EXCL_CHECKS));
	ctrlShowJoins.Attach(GetDlgItem(IDC_SHOW_JOINS));
	ctrlSuppressMsg.Attach(GetDlgItem(IDC_SUPPRESS_FAV_CHAT_AND_PM));

	ctrlExclChecks.SetCheck(entry->getExclChecks() ? BST_CHECKED : BST_UNCHECKED);
	ctrlShowJoins.SetCheck(entry->getShowJoins() ? BST_CHECKED : BST_UNCHECKED);
	ctrlSuppressMsg.SetCheck(entry->getSuppressChatAndPM() ? BST_CHECKED : BST_UNCHECKED);

	ctrlSearchOverride.Attach(GetDlgItem(IDC_OVERRIDE_DEFAULT));
	CUpDownCtrl(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_SPIN)).SetRange(2, 120);
	CUpDownCtrl(GetDlgItem(IDC_FAV_SEARCH_PASSIVE_INTERVAL_SPIN)).SetRange(2, 120);

	ctrlSearchActive.Attach(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_BOX));
	ctrlSearchPassive.Attach(GetDlgItem(IDC_FAV_SEARCH_PASSIVE_INTERVAL_BOX));

	int searchActive = entry->getSearchInterval();
	int searchPassive = entry->getSearchIntervalPassive();
	if (searchActive || searchPassive)
	{
		ctrlSearchActive.EnableWindow(TRUE);
		ctrlSearchPassive.EnableWindow(TRUE);
		ctrlSearchOverride.SetCheck(BST_CHECKED);
	}
	else
	{
		ctrlSearchActive.EnableWindow(FALSE);
		ctrlSearchPassive.EnableWindow(FALSE);
		ctrlSearchOverride.SetCheck(BST_UNCHECKED);
	}
	if (!searchActive || !searchPassive)
	{
		auto ss = SettingsManager::instance.getCoreSettings();
		ss->lockRead();
		if (!searchActive) searchActive = ss->getInt(Conf::MIN_SEARCH_INTERVAL);
		if (!searchPassive) searchPassive = ss->getInt(Conf::MIN_SEARCH_INTERVAL_PASSIVE);
		ss->unlockRead();
	}

	ctrlSearchActive.SetWindowText(Util::toStringT(searchActive).c_str());
	ctrlSearchPassive.SetWindowText(Util::toStringT(searchPassive).c_str());

	return FALSE;
}

LRESULT FavoriteHubTabOptions::onChangeSearchCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL state = ctrlSearchOverride.GetCheck() == BST_CHECKED;
	ctrlSearchActive.EnableWindow(state);
	ctrlSearchPassive.EnableWindow(state);
	return 0;
}

LRESULT FavoriteHubTabCheats::onInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
	DialogLayout::layout(m_hWnd, layoutItemsCheats, _countof(layoutItemsCheats));

	ctrlFakeHubCount.Attach(GetDlgItem(IDC_EXCLUSIVE_HUB));
	ctrlFakeHubCount.SetCheck(entry->getExclusiveHub() ? BST_CHECKED : BST_UNCHECKED);

	ctrlEnableFakeShare.Attach(GetDlgItem(IDC_FAKE_SHARE));
	ctrlFakeShare.Attach(GetDlgItem(IDC_FAKE_SHARE_SIZE));
	ctrlFakeShareUnit.Attach(GetDlgItem(IDC_SIZE_TYPE));
	ctrlFakeCount.Attach(GetDlgItem(IDC_FILE_COUNT));
	ctrlRandomCount.Attach(GetDlgItem(IDC_WIZARD_NICK_RND));
	ctrlEnableFakeSlots.Attach(GetDlgItem(IDC_FAKE_SLOTS));
	ctrlFakeSlots.Attach(GetDlgItem(IDC_FAKE_SLOTS_EDIT));
	ctrlOverrideStatus.Attach(GetDlgItem(IDC_OVERRIDE_STATUS));
	ctrlFakeStatus.Attach(GetDlgItem(IDC_FAKE_STATUS));

	static const ResourceManager::Strings sizeUnitStrings[] =
	{
		R_(B), R_(KB), R_(MB), R_(GB), R_(TB), R_INVALID
	};
	WinUtil::fillComboBoxStrings(ctrlFakeShareUnit, sizeUnitStrings);
	int fakeShareUnit = 0;
	const string& fakeShare = entry->getFakeShare();
	BOOL useFakeShare = FALSE;
	if (!fakeShare.empty())
	{
		double fakeShareSize;
		FavoriteHubEntry::parseSizeString(fakeShare, &fakeShareSize, &fakeShareUnit);
		if (fakeShareSize >= 0)
		{
			useFakeShare = TRUE;
			ctrlFakeShare.SetWindowText(Util::toStringT(fakeShareSize).c_str());
		}
	}

	static const ResourceManager::Strings clientStatusStrings[] =
	{
		R_(CLIENT_STATUS_NORMAL), R_(FILESERVER), R_(FIREBALL), R_INVALID
	};
	WinUtil::fillComboBoxStrings(ctrlFakeStatus, clientStatusStrings);

	ctrlFakeShareUnit.SetCurSel(fakeShareUnit);
	ctrlEnableFakeShare.SetCheck(useFakeShare ? BST_CHECKED : BST_UNCHECKED);
	ctrlFakeShare.EnableWindow(useFakeShare);
	ctrlFakeShareUnit.EnableWindow(useFakeShare);
	ctrlFakeCount.EnableWindow(useFakeShare);
	ctrlRandomCount.EnableWindow(useFakeShare);

	if (entry->getFakeFileCount() > 0)
		ctrlFakeCount.SetWindowText(Util::toStringT(entry->getFakeFileCount()).c_str());

	int fakeSlots = entry->getFakeSlots();
	if (fakeSlots >= 0)
	{
		ctrlFakeSlots.SetWindowText(Util::toStringT(fakeSlots).c_str());
		ctrlFakeSlots.EnableWindow(TRUE);
		ctrlEnableFakeSlots.SetCheck(BST_CHECKED);
	}
	else
	{
		ctrlFakeSlots.EnableWindow(FALSE);
		ctrlEnableFakeSlots.SetCheck(BST_UNCHECKED);
	}

	int fakeStatus = entry->getFakeClientStatus();
	if (fakeStatus < 0 || fakeStatus > 3) fakeStatus = 0;
	ctrlOverrideStatus.SetCheck(fakeStatus ? BST_CHECKED : BST_UNCHECKED);
	ctrlFakeStatus.EnableWindow(fakeStatus != 0);
	if (fakeStatus) fakeStatus--;
	ctrlFakeStatus.SetCurSel(fakeStatus);

	return 0;
}

LRESULT FavoriteHubTabCheats::onChangeFakeShare(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL state = ctrlEnableFakeShare.GetCheck() == BST_CHECKED;
	ctrlFakeShare.EnableWindow(state);
	ctrlFakeShareUnit.EnableWindow(state);
	ctrlFakeCount.EnableWindow(state);
	ctrlRandomCount.EnableWindow(state);
	return 0;
}

LRESULT FavoriteHubTabCheats::onChangeFakeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL state = ctrlEnableFakeSlots.GetCheck() == BST_CHECKED;
	ctrlFakeSlots.EnableWindow(state);
	return 0;
}

LRESULT FavoriteHubTabCheats::onChangeOverrideStatus(WORD, WORD, HWND, BOOL&)
{
	BOOL state = ctrlOverrideStatus.GetCheck() == BST_CHECKED;
	ctrlFakeStatus.EnableWindow(state);
	return 0;
}

LRESULT FavoriteHubTabCheats::onRandomFileCount(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ctrlFakeCount.SetWindowText(Util::toStringT(Util::rand(1, 200000)).c_str());
	return 0;
}

LRESULT FavoriteHubTabAdvanced::onInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
	WinUtil::translate(*this, textsAdvanced);

	ctrlOpChat.Attach(GetDlgItem(IDC_OPCHAT));
	ctrlOpChat.SetWindowText(Text::toT(entry->getOpChat()).c_str());

	return FALSE;
}
