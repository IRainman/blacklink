#include <stdafx.h>
#include "RichTextLabel.h"
#include <atlmisc.h>
#include "WinUtil.h"
#include "Colors.h"
#include "BackingStore.h"
#include "../client/StrUtil.h"
#include "../client/BaseUtil.h"

enum
{
	MARGIN_LEFT,
	MARGIN_TOP,
	MARGIN_RIGHT,
	MARGIN_BOTTOM
};

static const int UNDERLINE_OFFSET_Y = 1;
static const int UNDERLINE_OFFSET_X = 0;

static const int MIN_FONT_SIZE = 6;
static const int MAX_FONT_SIZE = 72;

RichTextLabel::~RichTextLabel()
{
	cleanup();
	clearStyles();
}

BOOL RichTextLabel::SubclassWindow(HWND hWnd)
{
	ATLASSERT(m_hWnd == NULL);
	ATLASSERT(::IsWindow(hWnd));
	font = CWindow(hWnd).GetFont();
	if (!CWindowImpl<RichTextLabel>::SubclassWindow(hWnd)) return FALSE;
	WinUtil::getWindowText(hWnd, text);
	initialize();
	return TRUE;
}

LRESULT RichTextLabel::onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
	initialize();
	return lRes;
}

LRESULT RichTextLabel::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	cleanup();
	clearStyles();
	return 0;
}

LRESULT RichTextLabel::onSetText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	text.assign(reinterpret_cast<const TCHAR*>(lParam));
	initialize();
	return TRUE;
}

LRESULT RichTextLabel::onSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	font = (HFONT) wParam;
	return 0;
}

LRESULT RichTextLabel::onGetFont(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return (LRESULT) font;
}

void RichTextLabel::drawUnderline(HDC dc, int xStart, int xEnd, int y, int yBottom, COLORREF color) const
{
	y += UNDERLINE_OFFSET_Y;
	if (y >= yBottom) return;
	HPEN hPen = CreatePen(PS_SOLID, 1, color);
	HGDIOBJ oldPen = SelectObject(dc, hPen);
	MoveToEx(dc, xStart - UNDERLINE_OFFSET_X, y, nullptr);
	LineTo(dc, xEnd, y);
	SelectObject(dc, oldPen);
	DeleteObject(hPen);
}

LRESULT RichTextLabel::onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	PAINTSTRUCT ps;
	HDC paintDC = BeginPaint(&ps);

	CRect rc;
	GetClientRect(&rc);
	int width = rc.Width();
	int height = rc.Height();
	if (width <= 0 || height <= 0)
	{
		EndPaint(&ps);
		return 0;
	}

	HDC hdc = paintDC;
	if (!backingStore)
		backingStore = BackingStore::getBackingStore();
	if (backingStore)
	{
		HDC hMemDC = backingStore->getCompatibleDC(paintDC, rc.right, rc.bottom);
		if (hMemDC) hdc = hMemDC;
	}
	if (calcSizeFlag)
		calcSize(hdc);
	if (layoutFlag)
		layout(hdc, width);

	if (!bgBrush)
		bgBrush = CreateSolidBrush(useSystemColors ? GetSysColor(COLOR_BTNFACE) : colorBackground);
	if (useDialogBackground)
	{
		if (FAILED(DrawThemeParentBackground(m_hWnd, hdc, nullptr)))
			FillRect(hdc, &rc, bgBrush);
	}
	else
		FillRect(hdc, &rc, bgBrush);
	SetBkMode(hdc, TRANSPARENT);

	HRGN hrgn = nullptr;
	int bottom = rc.bottom;
	if (margins[MARGIN_LEFT] || margins[MARGIN_TOP] || margins[MARGIN_RIGHT] || margins[MARGIN_BOTTOM])
	{
		RECT rcClip = { margins[MARGIN_LEFT], margins[MARGIN_TOP], width - margins[MARGIN_RIGHT], height - margins[MARGIN_BOTTOM] };
		hrgn = CreateRectRgnIndirect(&rcClip);
		SelectClipRgn(hdc, hrgn);
		bottom -= margins[MARGIN_BOTTOM];
	}

	int underlinePos = -1;
	HFONT oldFont = nullptr;
	for (size_t i = 0; i < fragments.size(); ++i)
	{
		const Fragment& fragment = fragments[i];
		if (fragment.y >= bottom) break;
		if (fragment.text[0] == _T('\n')) continue;
		const StyleInstance& style = styles[fragment.style];
		HFONT prevFont = (HFONT) SelectObject(hdc, style.hFont);
		if (!oldFont) oldFont = prevFont;
		COLORREF color;
		if (fragment.link != -1)
			color = fragment.link == hoverLink ? colorLinkHover : colorLink;
		else
			color = style.color;
		SetTextColor(hdc, color);
		ExtTextOut(hdc, fragment.x, fragment.y, ETO_CLIPPED, &rc, fragment.text.c_str(), static_cast<UINT>(fragment.text.length()), nullptr);
		if ((style.font.lfUnderline || (fragment.link != -1 && fragment.link == hoverLink)) && underlinePos == -1)
			underlinePos = static_cast<int>(i);
	}
	if (underlinePos != -1)
	{
		const Fragment& fragment = fragments[underlinePos];
		const StyleInstance& style = styles[fragment.style];
		int y = fragment.y + style.metric.tmAscent;
		int xStart = fragment.x;
		int xEnd = 0;
		bool isLink = fragment.link != -1;
		for (size_t i = underlinePos; i < fragments.size(); ++i)
		{
			const Fragment& fragment = fragments[i];
			const StyleInstance& style = styles[fragment.style];
			if (style.font.lfUnderline || (fragment.link != -1 && fragment.link == hoverLink))
			{
				int yBaseLine = fragment.y + style.metric.tmAscent;
				if (xStart == -1)
				{
					y = yBaseLine;
					xStart = fragment.x;
					xEnd = xStart + fragment.width - fragment.spaceWidth;
					isLink = fragment.link != -1;
				}
				else if (yBaseLine == y)
				{
					xEnd = fragment.x + fragment.width - fragment.spaceWidth;
				}
				else
				{
					if (fragment.x > xEnd) xEnd = fragment.x;
					if (y >= bottom) break;
					drawUnderline(hdc, xStart, xEnd, y, bottom, isLink ? colorLinkHover : style.color);
					y = yBaseLine;
					xStart = fragment.x;
					xEnd = fragment.x + fragment.width - fragment.spaceWidth;
					isLink = fragment.link != -1;
				}
			}
			else if (xStart != -1)
			{
				if (y >= bottom) break;
				drawUnderline(hdc, xStart, xEnd, y, bottom, isLink ? colorLinkHover : style.color);
				xStart = -1;
			}
		}
		if (xStart != -1 && y < bottom)
		{
			const Fragment& fragment = fragments.back();
			COLORREF color = styles[fragment.style].color;
			drawUnderline(hdc, xStart, fragment.x + fragment.width - fragment.spaceWidth,
				y, bottom, isLink ? colorLinkHover : color);
		}
	}

	if (hrgn)
	{
		SelectClipRgn(hdc, nullptr);
		DeleteObject(hrgn);
	}
	if (oldFont) SelectObject(hdc, oldFont);
	if (hdc != paintDC) BitBlt(paintDC, rc.left, rc.top, width, height, hdc, 0, 0, SRCCOPY);

	EndPaint(&ps);
	return 0;
}

LRESULT RichTextLabel::onSize(UINT, WPARAM, LPARAM, BOOL&)
{
	layoutFlag = true;
	return 0;
}

LRESULT RichTextLabel::onMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/)
{
	if (tooltipActive)
	{
		MSG msg = { m_hWnd, uMsg, wParam, lParam };
		tooltip.RelayEvent(&msg);
	}

	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);

	CRect rc;
	GetClientRect(&rc);

	int newLinkFragment = -1;
	int newLink = -1;
	int bottom = rc.bottom - margins[MARGIN_BOTTOM];
	for (size_t i = 0; i < fragments.size(); ++i)
	{
		const Fragment& fragment = fragments[i];
		if (fragment.y >= bottom) break;
		if (fragment.text[0] == _T('\n')) continue;
		if (fragment.link == -1) continue;
		if (x >= fragment.x && x < fragment.x + fragment.width &&
		    y >= fragment.y && y < fragment.y + fragment.height)
		{
			newLink = fragment.link;
			newLinkFragment = static_cast<int>(i);
			break;
		}
	}

	if (newLink != hoverLink)
	{
		hoverLink = newLink;
		clickLink = -1;
		SendMessage(WM_SETCURSOR, (WPARAM) m_hWnd, 0);
		Invalidate();
		if (hoverLink != -1)
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = m_hWnd;
			_TrackMouseEvent(&tme);
		}
		if (useLinkTooltips) initTooltip(newLinkFragment);
	}

	return TRUE;
}

LRESULT RichTextLabel::onMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	clickLink = -1;
	if (hoverLink != -1)
	{
		if (tooltipActive)
		{
			tooltip.Activate(FALSE);
			tooltipActive = false;
		}
		hoverLink = -1;
		Invalidate();
	}
	return TRUE;
}

LRESULT RichTextLabel::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL & /*bHandled*/)
{
	clickLink = hoverLink;
	return 0;
}

LRESULT RichTextLabel::onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL & /*bHandled*/)
{
	if (clickLink >= 0 && clickLink < static_cast<int>(links.size()))
	{
		HWND parent = GetParent();
		::SendMessage(parent, WMU_LINK_ACTIVATED, 0, (LPARAM) links[clickLink].c_str());
		clickLink = -1;
	}
	return 0;
}

LRESULT RichTextLabel::onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetCursor(hoverLink != -1 ? linkCursor : defaultCursor);
	return TRUE;
}

LRESULT RichTextLabel::onNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	NMHDR* hdr = reinterpret_cast<NMHDR*>(lParam);
	if (hdr->code == TTN_GETDISPINFO && hdr->hwndFrom == tooltip && hoverLink != -1)
	{
		TOOLTIPTEXT* tt = reinterpret_cast<TOOLTIPTEXT*>(lParam);
		tt->lpszText = const_cast<TCHAR*>(links[hoverLink].c_str());
		return 0;
	}
	bHandled = FALSE;
	return 0;
}

LRESULT RichTextLabel::onThemeChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (bgBrush)
	{
		DeleteObject(bgBrush);
		bgBrush = nullptr;
	}
	Invalidate();
	return 0;
}

static inline bool isWhiteSpace(TCHAR c)
{
	return c == _T(' ') || c == _T('\t') || c == _T('\n') || c == _T('\r');
}

inline bool styleEq(const RichTextLabel::Style& s1, const RichTextLabel::Style& s2)
{
	return s1.font.lfWeight == s2.font.lfWeight &&
		s1.font.lfHeight == s2.font.lfHeight &&
		s1.font.lfItalic == s2.font.lfItalic &&
		s1.font.lfUnderline == s2.font.lfUnderline &&
		s1.color == s2.color;
}

int RichTextLabel::getStyle()
{
	if (!initialStyle.font.lfHeight)
	{
		GetObject(font ? font : (HFONT) GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &initialStyle.font);
		if (fontHeight) initialStyle.font.lfHeight = fontHeight;
	}

	int style = -1;
	const Style& currentStyle = tagStack.empty() ? initialStyle : tagStack.back().style;
	for (vector<StyleInstance>::size_type i = 0; i < styles.size(); ++i)
		if (styleEq(currentStyle, styles[i]))
		{
			style = static_cast<int>(i);
			break;
		}
	if (style < 0)
	{
		StyleInstance newStyle;
		newStyle.font = currentStyle.font;
		newStyle.color = currentStyle.color;
		newStyle.hFont = nullptr;
		style = static_cast<int>(styles.size());
		styles.push_back(newStyle);
	}
	return style;
}

void RichTextLabel::addFragment(tstring::size_type start, tstring::size_type end, bool whiteSpace)
{
	int style = getStyle();
	Fragment fragment;
	fragment.text = text.substr(start, end - start);
	unescape(fragment.text);
	if (whiteSpace)
		fragment.text += _T(' ');
	if (!fragments.empty())
	{
		Fragment& lastFragment = fragments.back();
		if (lastFragment.style == style && lastFragment.link == parsingLink &&
		    lastFragment.text.back() != _T(' '))
		{
			lastFragment.text += fragment.text;
			return;
		}
	}
	fragment.style = style;
	fragment.x = fragment.y = 0;
	fragment.width = fragment.height = fragment.spaceWidth = 0;
	fragment.link = parsingLink;
	fragments.push_back(fragment);
	lastFragment = static_cast<int>(fragments.size()) - 1;
}

void RichTextLabel::initialize()
{
	if (!linkCursor)
		linkCursor = LoadCursor(nullptr, IDC_HAND);
	if (!defaultCursor)
		defaultCursor = LoadCursor(nullptr, IDC_ARROW);

	memset(&initialStyle, 0, sizeof(initialStyle));
	tagStack.clear();
	fragments.clear();
	links.clear();
	clearStyles();
	parsingLink = -1;
	lastFragment = -1;
	initialStyle.color = useSystemColors ? GetSysColor(COLOR_BTNTEXT) : colorText;

	tstring::size_type start = tstring::npos;
	for (tstring::size_type i = 0; i < text.length(); ++i)
	{
		if (isWhiteSpace(text[i]))
		{
			if (lastFragment != -1 && fragments[lastFragment].text.back() != _T(' '))
				fragments[lastFragment].text += _T(' ');
			else if (!fragments.empty() && start == tstring::npos)
			{
				if (!(fragments.back().text[0] == _T('\n') && (text[i] == _T('\r') || text[i] == _T('\n'))))
					addFragment(i, i, true);
				continue;
			}
			if (start == tstring::npos) continue;
			addFragment(start, i, true);
			start = tstring::npos;
			continue;
		}
		if (text[i] == _T('<'))
		{
			if (start != tstring::npos)
			{
				addFragment(start, i, false);
				start = tstring::npos;
			}
			auto tagEnd = text.find(_T('>'), i + 1);
			if (tagEnd == tstring::npos) break;
			processTag(text.substr(i + 1, tagEnd - (i + 1)));
			i = tagEnd;
			lastFragment = -1;
			continue;
		}
		if (start == tstring::npos)
			start = i;
	}
	if (start != tstring::npos)
		addFragment(start, text.length(), false);
	calcSizeFlag = layoutFlag = true;
	tagStack.clear();
}

void RichTextLabel::getTextSize(HDC hdc, const tstring& text, int len, SIZE& size, const RichTextLabel::StyleInstance& style)
{
	if (len == 0)
	{
		size.cx = size.cy = 0;
		return;
	}
	GetTextExtentPoint32(hdc, text.c_str(), len, &size);
}

void RichTextLabel::calcSize(HDC hdc)
{
	HFONT oldFont = nullptr;
	for (auto& fragment : fragments)
	{
		StyleInstance& style = styles[fragment.style];
		bool getMetrics = false;
		if (!style.hFont)
		{
			getMetrics = true;
			auto underline = style.font.lfUnderline;
			style.font.lfUnderline = FALSE;
			style.hFont = CreateFontIndirect(&style.font);
			style.font.lfUnderline = underline;
		}
		if (fragment.text[0] == _T('\n')) continue;
		HFONT prevFont = (HFONT) SelectObject(hdc, style.hFont);
		if (!oldFont) oldFont = prevFont;
		if (getMetrics) GetTextMetrics(hdc, &style.metric);
		SIZE size;
		int len = static_cast<int>(fragment.text.length());
		getTextSize(hdc, fragment.text, len, size, style);
		fragment.width = size.cx;
		fragment.height = size.cy;
		if (fragment.text.back() == _T(' '))
		{
			while (len && fragment.text[len-1] == _T(' ')) len--;
			getTextSize(hdc, fragment.text, len, size, style);
			fragment.spaceWidth = fragment.width - size.cx;
		}
		else
			fragment.spaceWidth = 0;
	}
	if (oldFont) SelectObject(hdc, oldFont);
	calcSizeFlag = false;
	layoutFlag = true;
}

void RichTextLabel::layout(HDC hdc, int width)
{
	width -= margins[MARGIN_RIGHT];
	int x = margins[MARGIN_LEFT];
	int y = margins[MARGIN_TOP];
	int maxAscent = 0;
	int maxDescent = 0;
	size_t lineStart = 0;
	for (size_t i = 0; i < fragments.size(); ++i)
	{
		Fragment& fragment = fragments[i];
		const StyleInstance& style = styles[fragment.style];
		bool lineBreak = fragment.text[0] == _T('\n');
		if (lineBreak || (lineStart < i && x + fragment.width - fragment.spaceWidth > width))
		{
			if (lineBreak)
				updatePosition(style, maxAscent, maxDescent);
			for (size_t j = lineStart; j < i; ++j)
			{
				Fragment& fragment = fragments[j];
				int ascent = styles[fragment.style].metric.tmAscent;
				fragment.y = y + maxAscent - ascent;
			}
			if (centerFlag) centerLine(lineStart, i, width);
			x = margins[MARGIN_LEFT];
			y += maxAscent + maxDescent;
			maxAscent = maxDescent = 0;
			lineStart = i;
			if (lineBreak)
			{
				lineStart++;
				fragment.x = margins[MARGIN_LEFT];
				continue;
			}
		}
		updatePosition(style, maxAscent, maxDescent);
		fragment.x = x;
		x += fragment.width;
	}
	for (size_t j = lineStart; j < fragments.size(); ++j)
	{
		Fragment& fragment = fragments[j];
		const StyleInstance& style = styles[fragment.style];
		int ascent = style.metric.tmAscent;
		fragment.y = y + maxAscent - ascent;
	}
	if (centerFlag) centerLine(lineStart, fragments.size(), width);
	layoutFlag = false;
}

void RichTextLabel::updatePosition(const RichTextLabel::StyleInstance& style, int& maxAscent, int& maxDescent) const
{
	int top = style.metric.tmAscent;
	if (top > maxAscent) maxAscent = top;
	int bottom = style.metric.tmDescent;
	if (bottom > maxDescent) maxDescent = bottom;
}

void RichTextLabel::centerLine(size_t start, size_t end, int width)
{
	if (start >= end) return;
	const Fragment& fragment = fragments[end-1];
	int offset = (width - (fragment.x + fragment.width - fragment.spaceWidth)) / 2;
	for (size_t i = start; i < end; ++i)
		fragments[i].x += offset;
}

struct Attribute
{
	string name;
	string value;
};

static void parseTag(const string& s, string& tag, vector<Attribute>& attr)
{
	attr.clear();
	tag.clear();
	const char* c = s.c_str();
	bool getValue = false;
	Attribute a;
	string::size_type i = 0;
	while (i < s.length())
	{
		if (isWhiteSpace(c[i]))
		{
			++i;
			continue;
		}
		if (tag.empty())
		{
			string::size_type j = i + 1;
			while (j < s.length() && !isWhiteSpace(c[j])) ++j;
			tag = s.substr(i, j - i);
			Text::asciiMakeLower(tag);
			i = j;
			continue;
		}
		if (c[i] == '=' && !getValue && !attr.empty())
		{
			getValue = true;
			++i;
			continue;
		}
		if (getValue)
		{
			string::size_type j;
			if (c[i] == '\'')
			{
				j = s.find('\'', ++i);
				if (j == string::npos) break;
			}
			else if (c[i] == '"')
			{
				j = s.find('"', ++i);
				if (j == string::npos) break;
			}
			else
			{
				j = i;
				while (j < s.length() && !isWhiteSpace(c[j])) ++j;
			}
			attr.back().value = s.substr(i, j - i);
			getValue = false;
			i = j + 1;
			continue;
		}
		string::size_type j = i;
		while (j < s.length() && !isWhiteSpace(c[j]) && c[j] != '=') ++j;
		a.name = s.substr(i, j - i);
		if (a.name.empty()) break;
		Text::asciiMakeLower(a.name);
		attr.push_back(a);
		i = j;
		if (c[j] == '=')
		{
			getValue = true;
			++i;
		}
	}
}

static const string& getAttribValue(const vector<Attribute>& attr, const string& name)
{
	for (const auto& v : attr)
		if (v.name == name) return v.value;
	return Util::emptyString;
}

static void changeFontSize(LOGFONT& lf, int delta)
{
	int val = abs(lf.lfHeight) + delta;
	if (val < MIN_FONT_SIZE) val = MIN_FONT_SIZE; else
	if (val > MAX_FONT_SIZE) val = MAX_FONT_SIZE;
	if (val < 8) val = 8;
	if (lf.lfHeight < 0) val = -val;
	lf.lfHeight = val;
}

static void setFontSize(LOGFONT& lf, int val)
{
	if (val < MIN_FONT_SIZE) val = MIN_FONT_SIZE; else
	if (val > MAX_FONT_SIZE) val = MAX_FONT_SIZE;
	lf.lfHeight = -val;
}

void RichTextLabel::processTag(const tstring& data)
{
	string tag;
	vector<Attribute> attr;
	parseTag(Text::fromT(data), tag, attr);
	if (tag.empty()) return;
	if (tag == "br")
	{
		Fragment fragment;
		fragment.text = _T('\n');
		fragment.style = getStyle();
		fragment.x = fragment.y = 0;
		fragment.width = fragment.height = fragment.spaceWidth = 0;
		fragments.push_back(fragment);
		return;
	}
	if (tag[0] == '/')
	{
		tag.erase(0, 1);
		if (tagStack.empty() || tagStack.back().tag != tag) return;
		tagStack.pop_back();
		parsingLink = tagStack.empty() ? -1 : tagStack.back().link;
		return;
	}
	if (tag == "b")
	{
		TagStackItem item = newTagStackItem(tag);
		item.style.font.lfWeight = FW_BOLD;
		tagStack.push_back(item);
		return;
	}
	if (tag == "u")
	{
		TagStackItem item = newTagStackItem(tag);
		item.style.font.lfUnderline = TRUE;
		tagStack.push_back(item);
		return;
	}
	if (tag == "i" || tag == "em")
	{
		TagStackItem item = newTagStackItem(tag);
		item.style.font.lfItalic = TRUE;
		tagStack.push_back(item);
		return;
	}
	if (tag == "font")
	{
		TagStackItem item = newTagStackItem(tag);
		tagStack.push_back(item);
		const string& sizeValue = getAttribValue(attr, "size");
		if (!sizeValue.empty())
		{
			if (sizeValue[0] == '+')
			{
				auto val = Util::toUInt32(sizeValue.c_str() + 1);
				changeFontSize(tagStack.back().style.font, val);
			}
			else if (sizeValue[0] == '-')
			{
				auto val = Util::toUInt32(sizeValue.c_str() + 1);
				changeFontSize(tagStack.back().style.font, -(int) val);
			}
			else
			{
				auto val = Util::toUInt32(sizeValue);
				if (val) setFontSize(tagStack.back().style.font, val);
			}
		}
		const string& colorValue = getAttribValue(attr, "color");
		if (!colorValue.empty())
		{
			COLORREF color;
			if (Colors::getColorFromString(Text::toT(colorValue), color))
				tagStack.back().style.color = color;
		}
		return;
	}
	if (tag == "a")
	{
		TagStackItem item = newTagStackItem(tag);
		tagStack.push_back(item);
		const string& hrefValue = getAttribValue(attr, "href");
		parsingLink = static_cast<int>(links.size());
		tstring href = Text::toT(hrefValue);
		unescape(href);
		links.push_back(href);
		tagStack.back().link = parsingLink;
		return;
	}
}

RichTextLabel::TagStackItem RichTextLabel::newTagStackItem(const string& tag)
{
	TagStackItem item;
	item.tag = tag;
	item.link = -1;
	if (tagStack.empty())
	{
		if (!initialStyle.font.lfHeight)
		{
			GetObject(font ? font : (HFONT) GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &initialStyle.font);
			if (fontHeight) initialStyle.font.lfHeight = fontHeight;
		}
		item.style = initialStyle;
	}
	else
		item.style = tagStack.back().style;
	return item;
}

void RichTextLabel::unescape(tstring& text)
{
	tstring::size_type i = 0;
	while ((i = text.find(_T('&'), i)) != tstring::npos)
	{
		if (text.compare(i + 1, 3, _T("lt;"), 3) == 0)
			text.replace(i, 4, 1, _T('<'));
		else if (text.compare(i + 1, 4, _T("amp;"), 4) == 0)
			text.replace(i, 5, 1, _T('&'));
		else if (text.compare(i + 1, 3, _T("gt;"), 3) == 0)
			text.replace(i, 4, 1, _T('>'));
		else if (text.compare(i + 1, 5, _T("apos;"), 5) == 0)
			text.replace(i, 6, 1, _T('\''));
		else if (text.compare(i + 1, 5, _T("quot;"), 5) == 0)
			text.replace(i, 6, 1, _T('"'));
		i++;
	}
}

void RichTextLabel::cleanup()
{
	if (backingStore)
	{
		backingStore->release();
		backingStore = nullptr;
	}
	if (bgBrush)
	{
		DeleteObject(bgBrush);
		bgBrush = nullptr;
	}
}

void RichTextLabel::clearStyles()
{
	for (auto& style : styles)
		if (style.hFont) DeleteObject(style.hFont);
	styles.clear();
}

void RichTextLabel::setTextColor(COLORREF color)
{
	if (color == colorText) return;
	colorText = color;
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setBackgroundColor(COLORREF color)
{
	if (color == colorBackground) return;
	colorBackground = color;
	if (bgBrush)
	{
		DeleteObject(bgBrush);
		bgBrush = nullptr;
	}
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setLinkColor(COLORREF color, COLORREF colorHover)
{
	if (color == colorLink && colorHover == colorLinkHover) return;
	colorLink = color;
	colorLinkHover = colorHover;
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setMargins(int left, int top, int right, int bottom)
{
	bool update = false;
	if (margins[MARGIN_LEFT] != left)
	{
		margins[MARGIN_LEFT] = left;
		update = true;
	}
	if (margins[MARGIN_RIGHT] != right)
	{
		margins[MARGIN_RIGHT] = right;
		update = true;
	}
	if (margins[MARGIN_TOP] != top)
	{
		margins[MARGIN_TOP] = top;
		update = true;
	}
	if (margins[MARGIN_BOTTOM] != bottom)
	{
		margins[MARGIN_BOTTOM] = bottom;
		update = true;
	}
	if (!update) return;
	layoutFlag = true;
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setCenter(bool center)
{
	if (center == centerFlag) return;
	centerFlag = center;
	layoutFlag = true;
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setTextHeight(int height)
{
	int value = 0;
	if (height)
	{
		value = abs(height);
		if (value < MIN_FONT_SIZE) value = MIN_FONT_SIZE; else
		if (value > MAX_FONT_SIZE) value = MAX_FONT_SIZE;
		if (height < 0) value = -value;
	}
	if (fontHeight == value) return;
	fontHeight = value;
	initialize();
	if (m_hWnd) Invalidate();
}

void RichTextLabel::initTooltip(int fragment)
{
	if (tooltipActive)
	{
		tooltip.Activate(FALSE);
		tooltipActive = false;
	}
	if (!tooltip)
		tooltip.Create(m_hWnd, 0, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
	if (tooltipHasTool)
	{
		tooltip.DelTool(m_hWnd, 1);
		tooltipHasTool = false;
	}
	if (fragment != -1)
	{
		TOOLINFO tool = { sizeof(tool) };
		tool.hwnd = m_hWnd;
		tool.lpszText = LPSTR_TEXTCALLBACK;
		const Fragment& f = fragments[fragment];
		tool.rect.left = f.x;
		tool.rect.top = f.y;
		tool.rect.right = f.x + f.width;
		tool.rect.bottom = f.y + f.height;
		tool.uId = 1;
		if (tooltip.AddTool(&tool))
		{
			tooltip.Activate(TRUE);
			tooltipHasTool = tooltipActive = true;
		}
	}
}

void RichTextLabel::setUseDialogBackground(bool flag)
{
	if (flag && !IsAppThemed()) return;
	if (useDialogBackground == flag) return;
	useDialogBackground = flag;
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setUseSystemColors(bool flag)
{
	if (useSystemColors == flag) return;
	useSystemColors = flag;
	if (bgBrush)
	{
		DeleteObject(bgBrush);
		bgBrush = nullptr;
	}
	if (m_hWnd) Invalidate();
}

void RichTextLabel::setUseLinkTooltips(bool flag)
{
	useLinkTooltips = flag;
	if (!flag && tooltipActive)
	{
		tooltip.Activate(FALSE);
		tooltipActive = false;
	}
}
