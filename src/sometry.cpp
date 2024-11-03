// #include <boost/asio.hpp>
// #include <boost/asio/awaitable.hpp>
// #include <boost/asio/co_spawn.hpp>
// #include <boost/asio/detached.hpp>
// #include <boost/asio/io_context.hpp>
// #include <boost/asio/ip/tcp.hpp>
// #include <boost/asio/read_until.hpp>
// #include <boost/asio/redirect_error.hpp>
// #include <boost/asio/signal_set.hpp>
// #include <boost/asio/steady_timer.hpp>
// #include <boost/asio/use_awaitable.hpp>
// #include <boost/asio/write.hpp>
// #include <boost/cobalt.hpp>
// #include <boost/leaf.hpp>
// #include <cstdlib>
// #include <deque>
// #include <iostream>
// #include <list>
// #include <memory>
// #include <set>
// #include <string>
// #include <thread>
// #include <utility>

// using boost::asio::awaitable;
// using boost::asio::co_spawn;
// using boost::asio::detached;
// using boost::asio::redirect_error;
// using boost::asio::use_awaitable;
// using boost::asio::ip::tcp;
// namespace cobalt = boost::cobalt;
// namespace leaf = boost::leaf;

// //----------------------------------------------------------------------

// class chat_participant {
//   public:
// 	virtual ~chat_participant() {}
// 	virtual void deliver(const std::string &msg) = 0;
// };

// typedef std::shared_ptr<chat_participant> chat_participant_ptr;

// //----------------------------------------------------------------------

// class chat_room {
//   public:
// 	void join(chat_participant_ptr participant) {
// 		std::cout << "joined room\n";
// 		participants_.insert(participant);
// 		for (auto &msg : recent_msgs_)
// 			participant->deliver(msg);
// 	}

// 	void leave(chat_participant_ptr participant) {
// 		participants_.erase(participant);
// 	}

// 	void deliver(const std::string &msg) {
// 		recent_msgs_.push_back(msg);
// 		while (recent_msgs_.size() > max_recent_msgs)
// 			recent_msgs_.pop_front();

// 		std::cout << "sending" << msg << " for " << participants_.size()
// 				  << " clients\n";
// 		for (auto &participant : participants_)
// 			participant->deliver(msg);
// 	}

//   private:
// 	std::set<chat_participant_ptr> participants_;
// 	enum { max_recent_msgs = 100 };
// 	std::deque<std::string> recent_msgs_;
// };

// //----------------------------------------------------------------------

// class chat_session : public chat_participant,
// 					 public std::enable_shared_from_this<chat_session> {
//   public:
// 	chat_session(tcp::socket socket, chat_room &room)
// 		: socket_(std::move(socket)), timer_(socket_.get_executor()),
// 		  room_(room) {
// 		timer_.expires_at(std::chrono::steady_clock::time_point::max());
// 	}

// 	void do_start() {
// 		room_.join(shared_from_this());

// 		cobalt::spawn(
// 			socket_.get_executor(),
// 			[self = shared_from_this()]() -> cobalt::task<void> {
// 				return self->do_read();
// 			}(),
// 			detached);

// 		cobalt::spawn(
// 			socket_.get_executor(),
// 			[self = shared_from_this()]() -> cobalt::task<void> {
// 				return self->do_write();
// 			}(),
// 			detached);
// 	}

// 	void start() {
// 		room_.join(shared_from_this());

// 		co_spawn(
// 			socket_.get_executor(),
// 			[self = shared_from_this()] { return self->reader(); }, detached);

// 		co_spawn(
// 			socket_.get_executor(),
// 			[self = shared_from_this()] { return self->writer(); }, detached);
// 	}

// 	void deliver(const std::string &msg) {
// 		write_msgs_.push_back(msg);
// 		timer_.cancel_one();
// 	}

//   private:
// 	cobalt::task<void> do_read() {
// 		try {
// 			for (std::string read_msg;;) {
// 				std::size_t n = co_await boost::asio::async_read_until(
// 					socket_, boost::asio::dynamic_buffer(read_msg), "\n",
// 					cobalt::use_op);
// 				std::cout << "read message\n";

// 				room_.deliver(read_msg.substr(0, n));
// 				read_msg.erase(0, n);
// 			}
// 		} catch (const std::exception &e) {
// 			std::cerr << e.what() << '\n';
// 		}
// 	}

// 	awaitable<void> reader() {
// 		try {
// 			for (std::string read_msg;;) {
// 				std::size_t n = co_await boost::asio::async_read_until(
// 					socket_, boost::asio::dynamic_buffer(read_msg), "\n",
// 					use_awaitable);
// 				std::cout << "read message\n";

// 				room_.deliver(read_msg.substr(0, n));
// 				read_msg.erase(0, n);
// 			}
// 		} catch (std::exception &) {
// 			stop();
// 		}
// 	}

// 	cobalt::task<void> do_write() {
// 		try {
// 			while (socket_.is_open()) {
// 				if (write_msgs_.empty()) {
// 					boost::system::error_code ec;
// 					std::cout << "waiting to write\n";
// 					co_await timer_.async_wait(
// 						redirect_error(cobalt::use_op, ec));
// 				} else {
// 					co_await boost::asio::async_write(
// 						socket_, boost::asio::buffer(write_msgs_.front()),
// 						cobalt::use_op);
// 					std::cout << "wrote\n";
// 					write_msgs_.pop_front();
// 				}
// 			}
// 		} catch (std::exception &) {
// 			stop();
// 		}
// 	}
// 	awaitable<void> writer() {
// 		try {
// 			while (socket_.is_open()) {
// 				if (write_msgs_.empty()) {
// 					boost::system::error_code ec;
// 					std::cout << "waiting to write\n";
// 					co_await timer_.async_wait(
// 						redirect_error(use_awaitable, ec));
// 				} else {
// 					co_await boost::asio::async_write(
// 						socket_, boost::asio::buffer(write_msgs_.front()),
// 						use_awaitable);
// 					std::cout << "wrote\n";
// 					write_msgs_.pop_front();
// 				}
// 			}
// 		} catch (std::exception &) {
// 			stop();
// 		}
// 	}

// 	void stop() {
// 		room_.leave(shared_from_this());
// 		socket_.close();
// 		timer_.cancel();
// 	}

// 	tcp::socket socket_;
// 	boost::asio::steady_timer timer_;
// 	chat_room &room_;
// 	std::deque<std::string> write_msgs_;
// };

// //----------------------------------------------------------------------

// cobalt::task<void> do_listen(tcp::acceptor acceptor) {
// 	chat_room room;

// 	for (;;) {
// 		std::make_shared<chat_session>(
// 			co_await acceptor.async_accept(cobalt::use_op), room)
// 			->start();
// 	}
// }

// awaitable<void> listener(tcp::acceptor acceptor) {
// 	chat_room room;

// 	for (;;) {
// 		std::make_shared<chat_session>(
// 			co_await acceptor.async_accept(use_awaitable), room)
// 			->start();
// 	}
// }

// //-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// --
// //-- -- -- -- -- -- -- -- --

// class my_client {

//   public:
// 	my_client(boost::asio::io_context &io_ctx) : client_timer(io_ctx) {
// 		client_timer.expires_at(std::chrono::steady_clock::time_point::max());
// 	}

// 	// awaitable<void> writer(std::shared_ptr<tcp::socket> socket) {
// 	// 	try {
// 	// 		boost::asio::steady_timer wait_timer(socket->get_executor());

// 	// 		for (std::string data;;) {
// 	// 			if (!socket->is_open())
// 	// 				break;
// 	// 			// struct pollfd input[1] = {{.fd = 0, .events = POLLIN}};
// 	// 			// if (poll(input, 1, 1000 * 10)) {
// 	// 			// 	char c;
// 	// 			// 	while (std::cin.get(c) && c != '\n')
// 	// 			// 		data += c;
// 	// 			// 	data += '\r\n';
// 	// 			// }
// 	// 			data = "hello\n";

// 	// 			std::cout << "waiting to write\n";
// 	// 			co_await boost::asio::async_write(
// 	// 				*socket, boost::asio::buffer(data), use_awaitable);
// 	// 			std::cout << "wrote\n";

// 	// 			wait_timer.expires_after(std::chrono::milliseconds(1000));
// 	// 			co_await wait_timer.async_wait(use_awaitable);
// 	// 		}
// 	// 	} catch (const std::exception &e) {
// 	// 		std::cerr << "Writer exception: " << e.what() << "\n";
// 	// 		socket->close();
// 	// 	}
// 	// }

// 	// awaitable<void> reader(std::shared_ptr<tcp::socket> socket) {
// 	// 	try {
// 	// 		std::string read_msg;
// 	// 		for (;;) {
// 	// 			if (!socket->is_open())
// 	// 				break;

// 	// 			std::cout << "waiting to read\n";
// 	// 			std::size_t n = co_await boost::asio::async_read_until(
// 	// 				*socket, boost::asio::dynamic_buffer(read_msg), "\n",
// 	// 				use_awaitable);
// 	// 			std::cout << "Received: " << read_msg.substr(0, n);
// 	// 			read_msg.erase(0, n);
// 	// 		}
// 	// 	} catch (const std::exception &e) {
// 	// 		std::cerr << "Reader exception: " << e.what() << "\n";
// 	// 		socket->close();
// 	// 	}
// 	// }

// 	void start_input_thread(std::shared_ptr<tcp::socket> socket) {
// 		std::thread([socket]() {
// 			try {
// 				for (std::string line; std::getline(std::cin, line);) {
// 					line += '\n';
// 					boost::asio::async_write(
// 						*socket, boost::asio::buffer(line),
// 						[](const boost::system::error_code &ec,
// 						   std::size_t /*length*/) {
// 							if (ec) {
// 								std::cerr << "Write error: " << ec.message()
// 										  << "\n";
// 							}
// 						});
// 				}
// 			} catch (const std::exception &e) {
// 				std::cerr << "Input thread exception: " << e.what() << "\n";
// 			}
// 		}).detach();
// 	}

// 	cobalt::task<void> reader(std::shared_ptr<tcp::socket> socket) {
// 		try {
// 			std::string read_msg;
// 			for (;;) {
// 				if (!socket->is_open())
// 					break;
// 				std::cout << "waiting to read\n";
// 				std::size_t n = co_await boost::asio::async_read_until(
// 					*socket, boost::asio::dynamic_buffer(read_msg), "\n",
// 					cobalt::use_op);
// 				std::cout << "Received: " << read_msg.substr(0, n);
// 				read_msg.erase(0, n);
// 			}
// 		} catch (const std::exception &e) {
// 			std::cerr << "Reader exception: " << e.what() << "\n";
// 			socket->close();
// 		}
// 	}

// 	cobalt::task<void> connect_to_server(boost::asio::io_context &io_context,
// 										 const std::string &host,
// 										 const std::string &port) {
// 		tcp::resolver resolver(io_context);
// 		auto endpoints =
// 			co_await resolver.async_resolve(host, port, cobalt::use_op);

// 		auto socket = std::make_shared<tcp::socket>(io_context);
// 		co_await boost::asio::async_connect(*socket, endpoints, cobalt::use_op);

// 		std::cout << "Connected to server at " << host << ":" << port << "\n";

// 		start_input_thread(socket);

// 		cobalt::spawn(io_context, reader(socket), detached);
// 	}

//   private:
// 	boost::asio::steady_timer client_timer;
// };

// cobalt::main co_main(int argc, char *argv[]) {
// 	try {

// 		std::string service = argv[1];

// 		if (service == "server") {

// 			boost::asio::io_context io_context(1);

// 			cobalt::spawn(
// 				io_context,
// 				do_listen(tcp::acceptor(io_context, {tcp::v4(), 5000})),
// 				detached);

// 			boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
// 			signals.async_wait([&](auto, auto) { io_context.stop(); });

// 			io_context.run();
// 		} else if (service == "client") {
// 			boost::asio::io_context io_ctx(1);

// 			my_client client{io_ctx};

// 			cobalt::spawn(io_ctx,
// 						  client.connect_to_server(io_ctx, "127.0.0.1", "5000"),
// 						  detached);

// 			io_ctx.run();
// 		}

// 	} catch (std::exception &e) {
// 		std::cerr << "Exception: " << e.what() << "\n";
// 	}

// 	co_return 0;
// }

#include "chat_session.hpp"
#include "my_client.hpp"
#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <boost/cobalt/leaf.hpp>
#include <boost/leaf.hpp>
#include <iostream>

cobalt::task<void> do_listen(tcp::acceptor acceptor) {
	chat_room room;

	for (;;) {
		std::make_shared<chat_session>(
			co_await acceptor.async_accept(cobalt::use_op), room)
			->start();
	}
}

cobalt::main co_main(int argc, char *argv[]) {
	co_await cobalt::try_catch(
		[&]() -> cobalt::promise<void> {
			std::string service = argv[1];
			if (service == "server") {
				boost::asio::io_context io_context(1);
				cobalt::spawn(
					io_context,
					do_listen(tcp::acceptor(io_context, {tcp::v4(), 5001})),
					boost::asio::detached);

				boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
				signals.async_wait([&](auto, auto) { io_context.stop(); });

				io_context.run();
			} else if (service == "client") {
				boost::asio::io_context io_ctx(1);
				my_client client{io_ctx};
				client.connect_to_server(io_ctx, "127.0.0.1", "5001");
				io_ctx.run();
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

	// co_await cobalt::try_catch(
	// 	[&]() -> cobalt::promise<void> {

	// 	}(),
	// 	[](const boost::system::system_error &err) {
	// 		std::cout << "error: \n";
	// 		std::cout << err.what() << std::endl;
	// 	},
	// 	[](const boost::system::error_code &ec) {
	// 		std::cout << "error: \n";
	// 		std::cout << ec.what() << std::endl;
	// 	},
	// 	[](const boost::leaf::diagnostic_info &info) {
	// 		std::cout << "error: \n";
	// 		std::cout << info << std::endl;
	// 	});
	co_return 0;
}
