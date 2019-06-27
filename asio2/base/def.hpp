/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_DEF_HPP__
#define __ASIO2_DEF_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if !defined(NDEBUG) && !defined(_DEBUG) && !defined(DEBUG)
#	define NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace asio2
{

	//---------------------------------------------------------------------------------------------
	// global macro
	//---------------------------------------------------------------------------------------------
	#define ASIO2_DEFAULT_SEND_BUFFER_SIZE             (1024)
	#define ASIO2_DEFAULT_RECV_BUFFER_SIZE             (1024)

	enum class state : int8_t
	{
		stopped,
		stopping,
		starting,
		started,
		running,
	};

	#ifndef ASIO2_ASSERT
		#if (defined(_DEBUG) || defined(DEBUG))
			#define ASIO2_ASSERT(x) assert(x);
		#else
			#define ASIO2_ASSERT(x) 
		#endif
	#endif


	//---------------------------------------------------------------------------------------------
	// exception log
	//---------------------------------------------------------------------------------------------

	#if defined(ASIO2_DUMP_EXCEPTION_LOG)
		#define ASIO2_DUMP_EXCEPTION_LOG_IMPL asio2::logger::get().log_trace("exception (errno - %d) : \"%s\" %s %d\n", get_last_error(), get_last_error_desc().data(), __FILE__, __LINE__);
	#else
		#define ASIO2_DUMP_EXCEPTION_LOG_IMPL 
	#endif



	//---------------------------------------------------------------------------------------------
	// tcp used macro
	//---------------------------------------------------------------------------------------------
	#define ASIO2_DEFAULT_TCP_SILENCE_TIMEOUT             (60 * 60) // unit second
	#define ASIO2_DEFAULT_SSL_SHUTDOWN_TIMEOUT            (10)



	//---------------------------------------------------------------------------------------------
	// pack model used macro
	//---------------------------------------------------------------------------------------------

	// mean that current data length is not enough for a completed packet,it need more data
	static const std::size_t need_more = (std::size_t(-1));

	// mean that current data is invalid,may be the client is a invalid or hacker client,if get this 
	// return value,the session will be disconnect forced.
	static const std::size_t bad_data  = (std::size_t(-2));



	//---------------------------------------------------------------------------------------------
	// auto model used macro
	//---------------------------------------------------------------------------------------------

	#define ASIO2_DEFAULT_HEADER_FLAG                     ((uint8_t)0b011101) // 29
	#define ASIO2_MAX_HEADER_FLAG                         ((uint8_t)0b111111) // 63
	#define ASIO2_MAX_PACKET_SIZE                         ((uint32_t)0b00000011111111111111111111111111) // 0x03FF FFFF
	#define ASIO2_HEADER_FLAG_MASK                        ((uint32_t)0b00000000000000000000000000111111)
	#define ASIO2_HEADER_FLAG_BITS                        (6)



	//---------------------------------------------------------------------------------------------
	// udp used macro
	//---------------------------------------------------------------------------------------------
	#define ASIO2_DEFAULT_UDP_SILENCE_TIMEOUT             (60) // unit second



	//---------------------------------------------------------------------------------------------
	// icmp used macro
	//---------------------------------------------------------------------------------------------
	#define ASIO2_DEFAULT_ICMP_REPLY_TIMEOUT              (5000)



	//---------------------------------------------------------------------------------------------
	// http used macro
	//---------------------------------------------------------------------------------------------
	#define ASIO2_HTTP_ERROR_CODE_MASK                    (0x400000) // http_parser error code,we manual set the 22 bit to 1
	#define ASIO2_DEFAULT_HTTP_KEEPALIVE_TIMEOUT          (60 * 60) // unit second
	#define ASIO2_DEFAULT_HTTP_SILENCE_TIMEOUT            (60 * 60) // unit second
	#define ASIO2_DEFAULT_HTTP_MAX_REQUEST_COUNT          (100)

}

#endif // !__ASIO2_DEF_HPP__
