#include "chat_session.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>

chat_session::chat_session(tcp::socket socket, chat_room &room)
	: socket_(std::move(socket)), timer_(socket_.get_executor()), room_(room) {
	timer_.expires_at(std::chrono::steady_clock::time_point::max());
}

void chat_session::start() {
	room_.join(shared_from_this());

	cobalt::spawn(
		socket_.get_executor(),
		[self = shared_from_this()]() -> cobalt::task<void> {
			return self->reader();
		}(),
		boost::asio::detached);

	cobalt::spawn(
		socket_.get_executor(),
		[self = shared_from_this()]() -> cobalt::task<void> {
			return self->writer();
		}(),
		boost::asio::detached);
}

void chat_session::deliver(const std::string &msg) {
	write_msgs_.push_back(msg);
	timer_.cancel_one();
}

cobalt::task<void> chat_session::reader() {
	co_await cobalt::try_catch(
		[&]() -> cobalt::promise<void> {
			for (std::string read_msg;;) {
				std::size_t n = co_await boost::asio::async_read_until(
					socket_, boost::asio::dynamic_buffer(read_msg), "\n",
					cobalt::use_op);
				std::cout << "read message\n";

				room_.deliver(read_msg.substr(0, n));
				read_msg.erase(0, n);
			}
		}(),
		[this](const boost::system::system_error &err) {
			std::cout << "error: \n";
			std::cout << err.what() << std::endl;
			stop();
		},
		[this](const boost::system::error_code &ec) {
			std::cout << "error: \n";
			std::cout << ec.what() << std::endl;
			stop();
		},
		[this](const boost::leaf::diagnostic_info &info) {
			std::cout << "error: \n";
			std::cout << info << std::endl;
			stop();
		});
}

cobalt::task<void> chat_session::writer() {
	co_await cobalt::try_catch(
		[&]() -> cobalt::promise<void> {
			while (socket_.is_open()) {
				if (write_msgs_.empty()) {
					boost::system::error_code ec;
					std::cout << "waiting to write\n";
					co_await timer_.async_wait(
						boost::asio::redirect_error(cobalt::use_op, ec));
				} else {
					co_await boost::asio::async_write(
						socket_, boost::asio::buffer(write_msgs_.front()),
						cobalt::use_op);
					std::cout << "wrote\n";
					write_msgs_.pop_front();
				}
			}
		}(),
		[this](const boost::system::system_error &err) {
			std::cout << "error: \n";
			std::cout << err.what() << std::endl;
			stop();
		},
		[this](const boost::system::error_code &ec) {
			std::cout << "error: \n";
			std::cout << ec.what() << std::endl;
			stop();
		},
		[this](const boost::leaf::diagnostic_info &info) {
			std::cout << "error: \n";
			std::cout << info << std::endl;
			stop();
		});
}

void chat_session::stop() {
	room_.leave(shared_from_this());
	socket_.close();
	timer_.cancel();
}
