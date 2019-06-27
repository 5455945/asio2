/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_TCPS_SESSION_IMPL_HPP__
#define __ASIO2_TCPS_SESSION_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio2/base/session_impl.hpp>

namespace asio2
{

	class tcps_session_impl : public session_impl
	{
		template<class _session_impl_t> friend class tcp_acceptor_impl;

	public:
		/**
		 * @construct
		 */
		explicit tcps_session_impl(
			std::shared_ptr<url_parser>             url_parser_ptr,
			std::shared_ptr<listener_mgr>           listener_mgr_ptr,
			std::shared_ptr<asio::io_context>       io_context_ptr,
			std::shared_ptr<session_mgr>            session_mgr_ptr,
			std::shared_ptr<asio::ssl::context>     ssl_context_ptr = nullptr
		)
			: session_impl(url_parser_ptr, listener_mgr_ptr, io_context_ptr, session_mgr_ptr)
			, m_socket(*io_context_ptr, *ssl_context_ptr)
			, m_ssl_context_ptr(ssl_context_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~tcps_session_impl()
		{
		}

		/**
		 * @function : start this session for prepare to recv msg
		 */
		virtual bool start() override
		{
			m_state = state::starting;

			// reset the variable to default status
			m_fire_close_is_called.clear(std::memory_order_release);
			m_last_active_time = std::chrono::system_clock::now();
			m_connect_time     = std::chrono::system_clock::now();

			try
			{
				// set keeplive
				set_keepalive_vals(this->get_socket());

				// setsockopt SO_SNDBUF from url params
				if (m_url_parser_ptr->get_so_sndbuf_size() > 0)
				{
					asio::socket_base::send_buffer_size option(m_url_parser_ptr->get_so_sndbuf_size());
					this->get_socket().set_option(option);
				}

				// setsockopt SO_RCVBUF from url params
				if (m_url_parser_ptr->get_so_rcvbuf_size() > 0)
				{
					asio::socket_base::receive_buffer_size option(m_url_parser_ptr->get_so_rcvbuf_size());
					this->get_socket().set_option(option);
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}

			_post_handshake(shared_from_this());

			return true;
		}

		/**
		 * @function : stop session
		 * note : this function must be noblocking,if it's blocking,will cause circle lock in session_mgr's function stop_all and stop
		 */
		virtual void stop() override
		{
			if (m_state >= state::starting)
			{
				auto prev_state = m_state;
				m_state = state::stopping;

				// close the socket by post a event
				// asio don't allow operate the same socket in multi thread,if you close socket in one thread and another thread is 
				// calling socket's async_... function,it will crash.so we must care for operate the socket.when need close the
				// socket ,we use the strand to post a event,make sure the socket's close operation is in the same thread.
				try
				{
					auto this_ptr(shared_from_this());
					m_strand_ptr->post([this, this_ptr, prev_state]() mutable
					{
						try
						{
							// if the client socket is not closed forever,this async_shutdown callback also can't be called forever,
							// so we use a timer to force close the socket,then the async_shutdown callback will be called.
							m_timer.expires_from_now(std::chrono::seconds(ASIO2_DEFAULT_SSL_SHUTDOWN_TIMEOUT));
							m_timer.async_wait(m_strand_ptr->wrap([this, this_ptr, prev_state](const error_code & ec) mutable
							{
								if (ec)
									set_last_error(ec.value());

								if (prev_state == state::running)
								{
									_fire_close(this_ptr, get_last_error());
								}

								// call socket's close function to notify the _handle_recv function response with error > 0 ,then the socket 
								// can get notify to exit
								if (this->get_socket().is_open())
								{
									error_code ec;
									// when the lowest socket is closed,the ssl stream shutdown will returned.
									this->get_socket().shutdown(asio::socket_base::shutdown_both, ec);
									// must ensure the close function has been called,otherwise the _handle_recv will never return
									this->get_socket().close(ec);
									set_last_error(ec.value());
								}

								m_state = state::stopped;

								// remove this session from the session map
								m_session_mgr_ptr->stop(this_ptr);
							}));

							// call socket's close function to notify the _handle_recv function response with error > 0 ,then the socket 
							// can get notify to exit
							if (this->get_socket().is_open())
							{
								// when server call ssl stream sync shutdown first,if the client socket is not closed forever,then here shutdowm will blocking forever.
								m_socket.async_shutdown(m_strand_ptr->wrap([this, this_ptr](const error_code & ec)
								{
									// clost the timer
									m_timer.cancel();

									if (ec)
										set_last_error(ec.value());
								}));
							}
						}
						catch (system_error & e)
						{
							set_last_error(e.code().value());
						}
					});
				}
				catch (std::exception &) {}
			}

			session_impl::stop();
		}

		/**
		 * @function : whether the session is started
		 */
		virtual bool is_started() override
		{
			return ((m_state >= state::started) && this->get_socket().is_open());
		}

		/**
		 * @function : check whether the session is stopped
		 */
		virtual bool is_stopped() override
		{
			return ((m_state == state::stopped) && !this->get_socket().is_open());
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::shared_ptr<buffer<uint8_t>> & buf_ptr) override
		{
			if (is_started() && buf_ptr)
			{
				try
				{
					// must use strand.post to send data.why we should do it like this ? see udp_session._post_send.
					m_strand_ptr->post(std::bind(&tcps_session_impl::_post_send, this,
						shared_from_this(),
						buf_ptr
					));
					return true;
				}
				catch (std::exception &) {}
			}
			else if (!this->get_socket().is_open())
			{
				set_last_error((int)errcode::socket_not_ready);
			}
			else
			{
				set_last_error((int)errcode::invalid_parameter);
			}
			return false;
		}
#if defined(ASIO2_USE_HTTP)
		/**
		 * @function : send data
		 * just used for http protocol
		 */
		virtual bool send(const std::shared_ptr<http_msg> & http_msg_ptr) override
		{
			ASIO2_ASSERT(false);
			return false;
		}
#endif

	public:
		/**
		 * @function : get the socket
		 */
		inline asio::ip::tcp::socket::lowest_layer_type & get_socket()
		{
			return this->m_socket.lowest_layer();
		}
		/**
		 * @function : get the stream
		 */
		inline asio::ssl::stream<asio::ip::tcp::socket> & get_stream()
		{
			return this->m_socket;
		}

		/**
		 * @function : get the local address
		 */
		virtual std::string get_local_address() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().local_endpoint().address().to_string();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the local port
		 */
		virtual unsigned short get_local_port() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().local_endpoint().port();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

		/**
		 * @function : get the remote address
		 */
		virtual std::string get_remote_address() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().remote_endpoint().address().to_string();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the remote port
		 */
		virtual unsigned short get_remote_port() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().remote_endpoint().port();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

	protected:
		virtual void * _get_key() override
		{
			return static_cast<void *>(this);
		}

		virtual void _post_handshake(std::shared_ptr<session_impl> this_ptr)
		{
			m_socket.async_handshake(asio::ssl::stream_base::server,
				m_strand_ptr->wrap(std::bind(&tcps_session_impl::_handle_handshake, this,
					std::placeholders::_1, // error_code
					std::move(this_ptr)
				)));
		}

		virtual void _handle_handshake(const error_code & ec, std::shared_ptr<session_impl> this_ptr)
		{
			set_last_error(ec.value());

			if (!ec)
			{
				m_state = state::started;

				_fire_accept(this_ptr);

				// if user has called stop in the listener function,so we can't start continue.
				if (m_state == state::started)
				{
					m_state = state::running;

					if (!this->m_session_mgr_ptr->start(this_ptr))
					{
						this->stop();
						return;
					}

					// start the timer of check silence timeout
					if (m_url_parser_ptr->get_silence_timeout() > 0)
					{
						m_timer.expires_from_now(std::chrono::seconds(m_url_parser_ptr->get_silence_timeout()));
						m_timer.async_wait(
							m_strand_ptr->wrap(std::bind(&tcps_session_impl::_handle_timer, this,
								std::placeholders::_1, // error_code
								this_ptr
							)));
					}

					// to avlid the user call stop in another thread,then it may be m_socket.async_read_some and m_socket.close be called at the same time
					this->m_strand_ptr->post([this, this_ptr]()
					{
						this->_post_recv(std::move(this_ptr), std::make_shared<buffer<uint8_t>>(
							m_url_parser_ptr->get_recv_buffer_size(), malloc_recv_buffer(m_url_parser_ptr->get_recv_buffer_size()), 0));
					});
				}
			}
		}

		virtual void _handle_timer(const error_code & ec, std::shared_ptr<session_impl> this_ptr)
		{
			if (!ec)
			{
				// silence duration seconds not exceed the silence timeout,post a timer event agagin to avoid this session shared_ptr
				// object disappear.
				if (get_silence_duration() < m_url_parser_ptr->get_silence_timeout() * 1000)
				{
					auto remain = (m_url_parser_ptr->get_silence_timeout() * 1000) - get_silence_duration();
					m_timer.expires_from_now(std::chrono::milliseconds(remain));
					m_timer.async_wait(
						m_strand_ptr->wrap(std::bind(&tcps_session_impl::_handle_timer, this,
							std::placeholders::_1,
							std::move(this_ptr)
						)));
				}
				else
				{
					// silence timeout has elasped,but has't data trans,don't post a timer event again,so this session shared_ptr will
					// disappear and the object will be destroyed automatically after this handler returns.
					this->stop();
				}
			}
			else
			{
				// occur error,may be cancel is called
				this->stop();
			}
		}

		virtual void _post_recv(std::shared_ptr<session_impl> this_ptr, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (this->is_started())
			{
				if (buf_ptr->remain() > 0)
				{
					auto buffer = asio::buffer(buf_ptr->write_begin(), buf_ptr->remain());
					this->m_socket.async_read_some(buffer,
						this->m_strand_ptr->wrap(std::bind(&tcps_session_impl::_handle_recv, this,
							std::placeholders::_1, // error_code
							std::placeholders::_2, // bytes_recvd
							std::move(this_ptr),
							std::move(buf_ptr)
						)));
				}
				else
				{
					set_last_error((int)errcode::recv_buffer_size_too_small);
					ASIO2_DUMP_EXCEPTION_LOG_IMPL;
					this->stop();
					ASIO2_ASSERT(false);
				}
			}
		}

		virtual void _handle_recv(const error_code & ec, std::size_t bytes_recvd, std::shared_ptr<session_impl> this_ptr, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (!ec)
			{
				// every times recv data,we update the last active time.
				reset_last_active_time();

				buf_ptr->write_bytes(bytes_recvd);

				auto use_count = buf_ptr.use_count();

				_fire_recv(this_ptr, buf_ptr);

				if (use_count == buf_ptr.use_count())
				{
					buf_ptr->reset();
					this->_post_recv(std::move(this_ptr), std::move(buf_ptr));
				}
				else
				{
					this->_post_recv(std::move(this_ptr), std::make_shared<buffer<uint8_t>>(
						m_url_parser_ptr->get_recv_buffer_size(), malloc_recv_buffer(m_url_parser_ptr->get_recv_buffer_size()), 0));
				}
			}
			else
			{
				set_last_error(ec.value());

				this->stop();
			}

			// If an error occurs then no new asynchronous operations are started. This
			// means that all shared_ptr references to the connection object will
			// disappear and the object will be destroyed automatically after this
			// handler returns. The connection class's destructor closes the socket.
		}

		virtual void _post_send(std::shared_ptr<session_impl> this_ptr, std::shared_ptr<buffer<uint8_t>> buf_ptr)
		{
			if (is_started())
			{
				error_code ec;
				asio::write(m_socket, asio::buffer((void *)buf_ptr->read_begin(), buf_ptr->size()), ec);
				set_last_error(ec.value());
				this->_fire_send(this_ptr, buf_ptr, ec.value());
				if (ec)
				{
					ASIO2_DUMP_EXCEPTION_LOG_IMPL;
					this->stop();
				}
			}
			else
			{
				set_last_error((int)errcode::socket_not_ready);
				this->_fire_send(this_ptr, buf_ptr, get_last_error());
			}
		}

		virtual void _fire_accept(std::shared_ptr<session_impl> & this_ptr)
		{
			static_cast<server_listener_mgr *>(m_listener_mgr_ptr.get())->notify_accept(this_ptr);
		}

		virtual void _fire_recv(std::shared_ptr<session_impl> & this_ptr, std::shared_ptr<buffer<uint8_t>> & buf_ptr)
		{
			static_cast<server_listener_mgr *>(m_listener_mgr_ptr.get())->notify_recv(this_ptr, buf_ptr);
		}

		virtual void _fire_send(std::shared_ptr<session_impl> & this_ptr, std::shared_ptr<buffer<uint8_t>> & buf_ptr, int error)
		{
			static_cast<server_listener_mgr *>(m_listener_mgr_ptr.get())->notify_send(this_ptr, buf_ptr, error);
		}

		virtual void _fire_close(std::shared_ptr<session_impl> & this_ptr, int error)
		{
			if (!m_fire_close_is_called.test_and_set(std::memory_order_acquire))
			{
				static_cast<server_listener_mgr *>(m_listener_mgr_ptr.get())->notify_close(this_ptr, error);
			}
		}

	protected:
		/// ssl socket
		asio::ssl::stream<asio::ip::tcp::socket> m_socket;

		/// ssl context 
		std::shared_ptr<asio::ssl::context>      m_ssl_context_ptr;

		/// use to avoid call _fire_close twice
		std::atomic_flag m_fire_close_is_called = ATOMIC_FLAG_INIT;

	};

}

#endif // !__ASIO2_TCPS_SESSION_IMPL_HPP__
