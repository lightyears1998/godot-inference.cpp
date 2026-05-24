#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

struct LLMChatMessage : public godot::RefCounted {
	GDCLASS(LLMChatMessage, godot::RefCounted);

protected:
	static void _bind_methods();

public:
	godot::String role_;
	godot::String message_;

	godot::String role() const { return role_; }
	void set_role(const godot::String &role) { role_ = role; }
	godot::String message() const { return message_; }
	void set_message(const godot::String &message) { message_ = message; }
};
