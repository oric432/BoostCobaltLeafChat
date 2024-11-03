#include "my_client.hpp"
#include <boost/asio/write.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/leaf.hpp>
#include <boost/leaf.hpp>
#include <iostream>

namespace cobalt = boost::cobalt;

my_client::my_client(boost::asio::io_context &io_ctx) : client_timer(io_ctx) {
	client_timer.expires_at(std::chrono::steady_clock::time_point::max());
}

void my_client::start_input_thread(std::shared_ptr<tcp::socket> socket) {
	std::thread([socket]() {
		try {
			for (std::string line; std::getline(std::cin, line);) {
				line += '\n';
				boost::asio::async_write(*socket, boost::asio::buffer(line),
										 [](const boost::system::error_code &ec,
											std::size_t /*length*/) {
											 if (ec) {
												 std::cerr << "Write error: "
														   << ec.message()
														   << "\n";
											 }
										 });
			}
		} catch (const std::exception &e) {
			std::cerr << "Input thread exception: " << e.what() << "\n";
		}
	}).detach();
}

cobalt::detached my_client::reader(std::shared_ptr<tcp::socket> socket) {
	co_await cobalt::try_catch(
		[&]() -> cobalt::promise<void> {
			std::string read_msg;
			for (;;) {
				if (!socket->is_open())
					break;
				std::cout << "waiting to read\n";
				std::size_t n = co_await boost::asio::async_read_until(
					*socket, boost::asio::dynamic_buffer(read_msg), "\n",
					cobalt::use_op);
				std::cout << "Received: " << read_msg.substr(0, n);
				read_msg.erase(0, n);
			}

			co_return;
		}(),
		[](const boost::system::system_error &err) {
			std::cout << "error: \n";
			std::cout << err.what() << std::endl;
		},
		[](const boost::system::error_code &ec) {
			std::cout << "error: \n";
			std::cout << ec.what() << std::endl;
		},
		[](const boost::leaf::diagnostic_info &info) {
			std::cout << "error: \n";
			std::cout << info << std::endl;
		});
}

cobalt::detached
my_client::connect_to_server(boost::asio::io_context &io_context,
							 const std::string &host, const std::string &port) {
	tcp::resolver resolver(io_context);
	auto endpoints =
		co_await resolver.async_resolve(host, port, cobalt::use_op);

	auto socket = std::make_shared<tcp::socket>(io_context);
	co_await boost::asio::async_connect(*socket, endpoints, cobalt::use_op);

	std::cout << "Connected to server at " << host << ":" << port << "\n";

	start_input_thread(socket);
	reader(socket);
}
