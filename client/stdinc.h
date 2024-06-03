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

#ifndef DCPLUSPLUS_DCPP_STDINC_H
#define DCPLUSPLUS_DCPP_STDINC_H

#ifdef _WIN32
#include "w.h"
#endif

#include "compiler.h"

#include <wchar.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <stdint.h>
#include <limits.h>

#ifndef _WIN32
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <strings.h>
#endif

#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using std::vector;
using std::list;
using std::deque;
using std::pair;
using std::string;
using std::wstring;
using std::unique_ptr;
using std::min;
using std::max;

using std::make_pair;

inline int stricmp(const string& a, const string& b)
{
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

inline int strnicmp(const string& a, const string& b, size_t n)
{
#ifdef _WIN32
	return _strnicmp(a.c_str(), b.c_str(), n);
#else
	return strncasecmp(a.c_str(), b.c_str(), n);
#endif
}

inline int strnicmp(const char* a, const char* b, size_t n)
{
#ifdef _WIN32
	return _strnicmp(a, b, n);
#else
	return strncasecmp(a, b, n);
#endif
}

inline int stricmp(const wstring& a, const wstring& b)
{
#ifdef _WIN32
	return _wcsicmp(a.c_str(), b.c_str());
#else
	return wcscasecmp(a.c_str(), b.c_str());
#endif
}

inline int stricmp(const wchar_t* a, const wchar_t* b)
{
#ifdef _WIN32
	return _wcsicmp(a, b);
#else
	return wcscasecmp(a, b);
#endif
}

inline int strnicmp(const wstring& a, const wstring& b, size_t n)
{
#ifdef _WIN32
	return _wcsnicmp(a.c_str(), b.c_str(), n);
#else
	return wcsncasecmp(a.c_str(), b.c_str(), n);
#endif
}

inline int strnicmp(const wchar_t* a, const wchar_t* b, size_t n)
{
#ifdef _WIN32
	return _wcsnicmp(a, b, n);
#else
	return wcsncasecmp(a, b, n);
#endif
}

#ifdef _MSC_VER
#define alloca _alloca
#endif

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef _T
#ifdef _UNICODE
#define _T(x) L ## x
#else
#define _T(x) x
#endif
#endif

#endif // !defined(DCPLUSPLUS_DCPP_STDINC_H)
