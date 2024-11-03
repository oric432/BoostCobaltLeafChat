#pragma once
#include "chat_participant.hpp"
#include <deque>
#include <iostream>
#include <memory>
#include <set>
#include <string>

class chat_room {
  public:
	void join(chat_participant_ptr participant);
	void leave(chat_participant_ptr participant);
	void deliver(const std::string &msg);

  private:
	std::set<chat_participant_ptr> participants_;
	static constexpr size_t max_recent_msgs = 100;
	std::deque<std::string> recent_msgs_;
};
