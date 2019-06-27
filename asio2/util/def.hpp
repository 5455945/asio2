/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __UTIL_DEF_HPP__
#define __UTIL_DEF_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


//---------------------------------------------------------------------------------------------
// environment macro
//---------------------------------------------------------------------------------------------
#ifndef WINDOWS
#	if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_) || defined(WIN32)
#		define WINDOWS
#	endif
#endif

#ifndef WIN_X86
#	if !defined(_WIN64) && (defined(_WIN32) || defined(_WINDOWS_) || defined(WIN32))
#		define WIN_X86
#	endif
#endif

#ifndef WIN_X64
#	if defined(_WIN64)
#		define WIN_X64
#	endif
#endif

#ifndef LINUX
#	if defined(__unix__) || defined(__linux__)
#		define LINUX
#	endif
#endif

#ifndef LINUX_X86
#	if defined(LINUX) && defined(__i386__) && !defined(__x86_64__)
#		define LINUX_X86
#	endif
#endif

#ifndef LINUX_X64
#	if defined(LINUX) && defined(__x86_64__)
#		define LINUX_X64
#	endif
#endif

#ifndef X86
#	if defined(WIN_X86) || defined(LINUX_X86) || defined(_M_IX86)
#		define X86
#	endif
#endif

#ifndef X64
#	if defined(WIN_X64) || defined(LINUX_X64) || defined(_M_X64)
#		define X64
#	endif
#endif

#if defined( _M_ARM )
#	define A32
#elif defined( _M_ARM64 )
#	define A64
#endif

/// separator of folder
#ifndef SLASH
#	if defined(LINUX)
#		define SLASH ('/')
#	elif defined(WINDOWS)
#		define SLASH ('\\')
#	endif
#endif

/// separator of folder
#ifndef SLASH_STRING
#	if defined(LINUX)
#		define SLASH_STRING ("/")
#	elif defined(WINDOWS)
#		define SLASH_STRING ("\\")
#	endif
#endif

#ifndef PATH_MAX
#	if defined(LINUX)
#		define PATH_MAX PATH_MAX
#	elif defined(WINDOWS)
#		define PATH_MAX MAX_PATH
#	endif
#endif

#ifndef LN
#	if defined(LINUX)
#		define LN ("\n")
#	elif defined(WINDOWS)
#		define LN ("\r\n")
#	endif
#endif

#ifndef IP_MAXSIZE
#	define IP_MAXSIZE 40
#endif

//---------------------------------------------------------------------------------------------
// global function
//---------------------------------------------------------------------------------------------
#ifndef SAFE_DELETE
#	define SAFE_DELETE(p) { if(p){ delete (p); (p) = nullptr; } }
#endif



#endif // !__UTIL_DEF_HPP__
