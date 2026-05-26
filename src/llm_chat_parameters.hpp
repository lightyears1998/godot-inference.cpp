#pragma once

#include <godot_cpp/classes/resource.hpp>

class LLMChatParameters : public godot::Resource {
	GDCLASS(LLMChatParameters, godot::Resource);

public:
	LLMChatParameters() = default;

	int context_length_ = 32768;
	float temperature_ = 0.8f;
	float top_p_ = 1.0f;
	int top_k_ = 40;
	float min_p_ = 0.05f;
	float presence_penalty_ = 0.0f;
	float repeat_penalty_ = 1.1f;

	int context_length() const { return context_length_; }
	void set_context_length(int value) { context_length_ = value; }

	float temperature() const { return temperature_; }
	void set_temperature(float value) { temperature_ = value; }

	float top_p() const { return top_p_; }
	void set_top_p(float value) { top_p_ = value; }

	int top_k() const { return top_k_; }
	void set_top_k(int value) { top_k_ = value; }

	float min_p() const { return min_p_; }
	void set_min_p(float value) { min_p_ = value; }

	float presence_penalty() const { return presence_penalty_; }
	void set_presence_penalty(float value) { presence_penalty_ = value; }

	float repeat_penalty() const { return repeat_penalty_; }
	void set_repeat_penalty(float value) { repeat_penalty_ = value; }

protected:
	static void _bind_methods();
};
