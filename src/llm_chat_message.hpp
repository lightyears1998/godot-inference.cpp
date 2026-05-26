#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

class LLMChatMessage : public godot::RefCounted {
	GDCLASS(LLMChatMessage, godot::RefCounted);

public:
	LLMChatMessage() = default;
	LLMChatMessage(const godot::StringName &role, const godot::String &content);

	godot::StringName role_;
	godot::String message_;

	godot::StringName role() const { return role_; }
	void set_role(const godot::StringName &role) { role_ = role; }
	godot::String message() const { return message_; }
	void set_message(const godot::String &message) { message_ = message; }

protected:
	static void _bind_methods();
};
