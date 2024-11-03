#include "chat_room.hpp"

void chat_room::join(chat_participant_ptr participant) {
	std::cout << "joined room\n";
	participants_.insert(participant);
	for (auto &msg : recent_msgs_)
		participant->deliver(msg);
}

void chat_room::leave(chat_participant_ptr participant) {
	participants_.erase(participant);
}

void chat_room::deliver(const std::string &msg) {
	recent_msgs_.push_back(msg);
	while (recent_msgs_.size() > max_recent_msgs)
		recent_msgs_.pop_front();

	std::cout << "sending " << msg << " for " << participants_.size()
			  << " clients\n";
	for (auto &participant : participants_)
		participant->deliver(msg);
}
