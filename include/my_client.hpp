#pragma once
#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <memory>
#include <string>
#include <thread>

using boost::asio::awaitable;
using boost::asio::ip::tcp;

namespace cobalt = boost::cobalt;

class my_client {
  public:
	my_client(boost::asio::io_context &io_ctx);
	cobalt::detached connect_to_server(boost::asio::io_context &io_context,
									   const std::string &host,
									   const std::string &port);

  private:
	void start_input_thread(std::shared_ptr<tcp::socket> socket);
	cobalt::detached reader(std::shared_ptr<tcp::socket> socket);

	boost::asio::steady_timer client_timer;
};
