#include "llm_chat_parameters.hpp"

void LLMChatParameters::_bind_methods() {
	using namespace godot;

	ClassDB::bind_method(D_METHOD("get_context_length"), &LLMChatParameters::context_length);
	ClassDB::bind_method(D_METHOD("set_context_length", "value"), &LLMChatParameters::set_context_length);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "context_length"), "set_context_length", "get_context_length");

	ClassDB::bind_method(D_METHOD("get_temperature"), &LLMChatParameters::temperature);
	ClassDB::bind_method(D_METHOD("set_temperature", "value"), &LLMChatParameters::set_temperature);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature"), "set_temperature", "get_temperature");

	ClassDB::bind_method(D_METHOD("get_top_p"), &LLMChatParameters::top_p);
	ClassDB::bind_method(D_METHOD("set_top_p", "value"), &LLMChatParameters::set_top_p);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "top_p"), "set_top_p", "get_top_p");

	ClassDB::bind_method(D_METHOD("get_top_k"), &LLMChatParameters::top_k);
	ClassDB::bind_method(D_METHOD("set_top_k", "value"), &LLMChatParameters::set_top_k);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "top_k"), "set_top_k", "get_top_k");

	ClassDB::bind_method(D_METHOD("get_min_p"), &LLMChatParameters::min_p);
	ClassDB::bind_method(D_METHOD("set_min_p", "value"), &LLMChatParameters::set_min_p);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_p"), "set_min_p", "get_min_p");

	ClassDB::bind_method(D_METHOD("get_presence_penalty"), &LLMChatParameters::presence_penalty);
	ClassDB::bind_method(D_METHOD("set_presence_penalty", "value"), &LLMChatParameters::set_presence_penalty);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "presence_penalty"), "set_presence_penalty", "get_presence_penalty");

	ClassDB::bind_method(D_METHOD("get_repeat_penalty"), &LLMChatParameters::repeat_penalty);
	ClassDB::bind_method(D_METHOD("set_repeat_penalty", "value"), &LLMChatParameters::set_repeat_penalty);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "repeat_penalty"), "set_repeat_penalty", "get_repeat_penalty");
}
