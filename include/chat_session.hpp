#pragma once
#include "chat_participant.hpp"
#include "chat_room.hpp"
#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <deque>
#include <memory>

namespace cobalt = boost::cobalt;

using boost::asio::awaitable;
using boost::asio::ip::tcp;

class chat_session : public chat_participant,
					 public std::enable_shared_from_this<chat_session> {
  public:
	chat_session(tcp::socket socket, chat_room &room);
	void start();
	void deliver(const std::string &msg) override;

  private:
	cobalt::task<void> reader();
	cobalt::task<void> writer();
	void stop();

	tcp::socket socket_;
	boost::asio::steady_timer timer_;
	chat_room &room_;
	std::deque<std::string> write_msgs_;
};
