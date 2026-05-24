#pragma once

#include "llm_chat.hpp"

#include <expected>
#include <filesystem>
#include <functional>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <shared_mutex>

class LLMConversationParameters : public godot::Resource {
	GDCLASS(LLMConversationParameters, Resource);

protected:
	static void _bind_methods();

private:
	float context_length_;
	float temperature_;
	float top_p_;
	float top_k_;
	float min_p_;
	float presence_penalty_;
	float repeat_penalty_;

	// -ctk q8_0 -ctv q8_0
};

class LLMModel : public godot::RefCounted {
	GDCLASS(LLMModel, RefCounted);

protected:
	static void _bind_methods();

public:
	godot::Ref<LLMChat> begin_chat(godot::Ref<LLMConversationParameters> params);
};
