/*
 * FlylinkDC++ // Chat Settings Page
 */

#include "stdafx.h"
#include "MessagesChatPage.h"
#include "WinUtil.h"
#include "DialogLayout.h"
#include "ConfUI.h"
#include "../client/ConfCore.h"

using DialogLayout::FLAG_TRANSLATE;
using DialogLayout::UNSPEC;
using DialogLayout::AUTO;

static const DialogLayout::Align align1 = { 2, DialogLayout::SIDE_RIGHT, U_DU(6) };
static const DialogLayout::Align align2 = { 3, DialogLayout::SIDE_LEFT, U_DU(4) };
static const DialogLayout::Align align3 = { 4, DialogLayout::SIDE_LEFT, 0 };

static const DialogLayout::Item layoutItems[] =
{
	{ IDC_PROTECT_PRIVATE, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_SETTINGS_PASSWORD, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_PM_PASSWORD_GENERATE, FLAG_TRANSLATE, UNSPEC, UNSPEC },
	{ IDC_PASSWORD, 0, UNSPEC, UNSPEC, 0, &align1, &align2 },
	{ IDC_PROTECT_PRIVATE_RND, FLAG_TRANSLATE, AUTO, UNSPEC, 0, &align3 },
	{ IDC_SETTINGS_PASSWORD_HINT, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_SETTINGS_PASSWORD_OK_HINT, FLAG_TRANSLATE, AUTO, UNSPEC },
	{ IDC_PROTECT_PRIVATE_SAY, FLAG_TRANSLATE, AUTO, UNSPEC }
};

static const PropPage::Item items[] =
{
	{ IDC_PROTECT_PRIVATE, Conf::PROTECT_PRIVATE, PropPage::T_BOOL },
	{ IDC_PASSWORD, Conf::PM_PASSWORD, PropPage::T_STR },
	{ IDC_PASSWORD_HINT, Conf::PM_PASSWORD_HINT, PropPage::T_STR },
	{ IDC_PASSWORD_OK_HINT, Conf::PM_PASSWORD_OK_HINT, PropPage::T_STR },
	{ IDC_PROTECT_PRIVATE_RND, Conf::PROTECT_PRIVATE_RND, PropPage::T_BOOL },
	{ IDC_PROTECT_PRIVATE_SAY, Conf::PROTECT_PRIVATE_SAY, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

static const PropPage::ListItem listItems[] =
{
	{ Conf::SHOW_SEND_MESSAGE_BUTTON, ResourceManager::SHOW_SEND_MESSAGE_BUTTON},
	{ Conf::SHOW_MULTI_CHAT_BTN, ResourceManager::SHOW_MULTI_CHAT_BTN },
#ifdef BL_UI_FEATURE_EMOTICONS
	{ Conf::SHOW_EMOTICONS_BTN, ResourceManager::SHOW_EMOTICONS_BTN },
#endif
#ifdef BL_UI_FEATURE_BB_CODES
	{ Conf::SHOW_BBCODE_PANEL, ResourceManager::SHOW_BBCODE_PANEL },
	{ Conf::SHOW_LINK_BTN, ResourceManager::SHOW_LINK_BTN },
#endif
	{ Conf::SHOW_TRANSCODE_BTN, ResourceManager::SHOW_TRANSCODE_BTN },
	{ Conf::SHOW_FIND_BTN, ResourceManager::SHOW_FIND_BTN },
	{ Conf::MULTILINE_CHAT_INPUT, ResourceManager::MULTILINE_CHAT_INPUT },
	{ Conf::MULTILINE_CHAT_INPUT_BY_CTRL_ENTER, ResourceManager::MULTILINE_CHAT_INPUT_BY_CTRL_ENTER },
#ifdef BL_UI_FEATURE_BB_CODES
	{ Conf::FORMAT_BB_CODES, ResourceManager::FORMAT_BB_CODES },
	{ Conf::FORMAT_BB_CODES_COLORS, ResourceManager::FORMAT_BB_CODES_COLORS },
	{ Conf::FORMAT_BOT_MESSAGE, ResourceManager::FORMAT_BOT_MESSAGE },
#endif
#ifdef BL_UI_FEATURE_EMOTICONS
	{ Conf::CHAT_ANIM_SMILES, ResourceManager::CHAT_ANIM_SMILES },
	{ Conf::SMILE_SELECT_WND_ANIM_SMILES, ResourceManager::SMILE_SELECT_WND_ANIM_SMILES },
#endif
	{ Conf::CHAT_PANEL_SHOW_INFOTIPS, ResourceManager::CHAT_PANEL_SHOW_INFOTIPS },
	{ Conf::CHAT_REFFERING_TO_NICK, ResourceManager::CHAT_REFFERING_TO_NICK },
	{ 0, ResourceManager::Strings() }
};

LRESULT MessagesChatPage::onInitDialog_chat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ctrlList.Attach(GetDlgItem(IDC_MESSAGES_CHAT_BOOLEANS));
	PropPage::read(*this, items, listItems, ctrlList);
	DialogLayout::layout(m_hWnd, layoutItems, _countof(layoutItems));

	ctrlTooltip.Create(m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP /*| TTS_BALLOON*/, WS_EX_TOPMOST);
	ctrlTooltip.SetDelayTime(TTDT_AUTOPOP, 15000);
	ATLASSERT(ctrlTooltip.IsWindow());
	ctrlSee.Attach(GetDlgItem(IDC_PROTECT_PRIVATE_SAY));
	WinUtil::addTool(ctrlTooltip, ctrlSee, ResourceManager::PROTECT_PRIVATE_SAY_TOOLTIP);
	ctrlProtect.Attach(GetDlgItem(IDC_PROTECT_PRIVATE));
	WinUtil::addTool(ctrlTooltip, ctrlProtect, ResourceManager::PROTECT_PRIVATE_TOOLTIP);
	ctrlRnd.Attach(GetDlgItem(IDC_PROTECT_PRIVATE_RND));
	WinUtil::addTool(ctrlTooltip, ctrlRnd, ResourceManager::PROTECT_PRIVATE_RND_TOOLTIP);
	ctrlTooltip.SetMaxTipWidth(256);
	ctrlTooltip.Activate(TRUE);

	fixControls();
	return TRUE;
}

void MessagesChatPage::write()
{
	PropPage::write(*this, items, listItems, GetDlgItem(IDC_MESSAGES_CHAT_BOOLEANS));
}

LRESULT MessagesChatPage::onEnablePassword(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	fixControls();
	return 0;
}

void MessagesChatPage::fixControls()
{
	BOOL enabled = ctrlProtect.GetCheck() == BST_CHECKED;
	GetDlgItem(IDC_PASSWORD).EnableWindow(enabled);
	GetDlgItem(IDC_PM_PASSWORD_GENERATE).EnableWindow(enabled);
	ctrlRnd.EnableWindow(enabled);
	GetDlgItem(IDC_PASSWORD_HINT).EnableWindow(enabled);
	GetDlgItem(IDC_PM_PASSWORD_HELP).EnableWindow(enabled);
	GetDlgItem(IDC_PASSWORD_OK_HINT).EnableWindow(enabled);
	ctrlSee.EnableWindow(enabled);
}

LRESULT MessagesChatPage::onRandomPassword(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SetDlgItemText(IDC_PASSWORD, Text::toT(Util::getRandomNick()).c_str());
	return 0;
}

LRESULT MessagesChatPage::onClickedHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */)
{
	MessageBox(CTSTRING(PRIVATE_PASSWORD_HELP), getAppNameVerT().c_str(), MB_OK | MB_ICONINFORMATION);
	return 0;
}
