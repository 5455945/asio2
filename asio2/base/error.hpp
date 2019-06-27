/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_ERROR_HPP__
#define __ASIO2_ERROR_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cerrno>
#include <iostream>
#include <memory>
#include <string>

#include <asio2/config.hpp>

#include <asio2/base/selector.hpp>
#include <asio2/base/def.hpp>
#include <asio2/util/logger.hpp>
#include <asio2/http/http_parser.h>

namespace asio2
{

	/**
	 * ssl error code is a unsigned int value,the highest 8 bits is the library code(more details,see openssl/err.h)
	 * 
	 * the asio2 custom error code rule : the highest 8 bits is 0,the 23 bit is 1,
	 *
	 * the http_parser error code rule : the highest 9 bits is 0,the 22 bit is 1,
	 *
	 * we check the error_category like this : 
	 * if the 23 bit is 1,it is asio2_category.
	 * if the 22 bit is 1,it is http_parser error code.
	 * if the error number's highest 8 bits is more than 0,it is ssl ssl_category.
	 * otherwise it is system_category.
	 */

	enum class errcode {	// names for generic error codes
		url_string_invalid         = 0x800001,
		recv_buffer_size_too_small = 0x800002,
		recvd_data_invalid         = 0x800003,
		send_data_failed           = 0x800004,
		socket_not_ready           = 0x800005,
		invalid_parameter          = 0x800006,
	};


	class asio2_category : public std::error_category
	{
	public:
		asio2_category() : std::error_category()
		{
		}
		virtual ~asio2_category()
		{
		}

		virtual const char *name() const noexcept
		{	// get name of category
			return ("asio2");
		}

		virtual std::string message(int err_num) const
		{	// convert to name of error

			static const char * _err_msg[] = {
				("the url string is invalid"),
				("the recv_buffer_size is too small to hold the packet"),
				("the received data is invalid"),
				("send data failed"),
				("the socket is closed or not ready"),
				("the parameter is incorrect"),
				("null"),
			};

			int index = err_num - 0x800001;
			return ((index >= 0 && index < (int)(sizeof(_err_msg) / sizeof(const char *))) ? _err_msg[index] : ("unknown error"));
		}
	};


	// use anonymous namespace to resolve global function redefinition problem
	namespace
	{

		/**
		 * thread local variable of error number.
		 */
		thread_local static int _err_num = 0;

		/**
		 * @function : get last error number
		 */
		inline int get_last_error()
		{
			return _err_num;
		}

		/**
		 * @function : set last error number
		 */
		inline void set_last_error(int err_num)
		{
			_err_num = err_num;
		}

		/**
		 * @function : get the specify error desc
		 */
		inline std::string get_error_desc(int err_num)
		{
			// 1.must check ssl_category first 
#if defined(ASIO2_USE_SSL)
			if (err_num & 0xff000000)
				return error_code(err_num, asio::error::get_ssl_category()).message();
#endif

			// 2.check asio2_category
			if (err_num & 0x00800000)
				return std::error_code(err_num, asio2_category()).message();
			
			// 3.check http_errno
			if (err_num & ASIO2_HTTP_ERROR_CODE_MASK)
				return http::parser::http_errno_description((enum http::parser::http_errno)(err_num & (~ASIO2_HTTP_ERROR_CODE_MASK)));

			return error_code(err_num, asio::error::get_system_category()).message();
		}

		/**
		 * @function : get last error desc
		 */
		inline std::string get_last_error_desc()
		{
			return get_error_desc(_err_num);
		}

	}


}

#endif // !__ASIO2_ERROR_HPP__
