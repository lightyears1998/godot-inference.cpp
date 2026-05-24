#pragma once

#include "llm_chat_message.hpp"

#include <expected>
#include <filesystem>
#include <functional>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <shared_mutex>

class LLMChat : public godot::RefCounted {
	GDCLASS(LLMChat, RefCounted)

protected:
	static void _bind_methods();

	enum ReplyStatus {
		OK,
		GENERATING
	};

public:
	void say(const godot::String &content);
	void remind(const godot::String &prompt);
	void think(const godot::String &prompt);
	void add_prompt(const godot::String &prompt);
	void finish_chat();
	void request_reply();
	ReplyStatus get_reply_status();
	godot::String get_last_reply();
	godot::TypedArray<LLMChatMessage> get_history();
};
