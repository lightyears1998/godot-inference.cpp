#include "llm_chat_message.hpp"

using namespace godot;

void LLMChatMessage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_role"), &LLMChatMessage::role);
	ClassDB::bind_method(D_METHOD("set_role", "p_role"), &LLMChatMessage::set_role);
	ClassDB::bind_method(D_METHOD("get_message"), &LLMChatMessage::message);
	ClassDB::bind_method(D_METHOD("set_message", "p_message"), &LLMChatMessage::set_message);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "role"), "set_role", "get_role");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");
}
