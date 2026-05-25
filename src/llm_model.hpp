#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <llama.h>
#include <memory>

class LLMChat;
class LLMChatParameters;
struct llama_model;
struct llama_context;

class LLMModel : public godot::RefCounted {
	GDCLASS(LLMModel, godot::RefCounted);

protected:
	static void _bind_methods();

public:
	LLMModel() = default;
	LLMModel(llama_model* model);

	godot::Ref<LLMChat> start_chat(const godot::Ref<LLMChatParameters>& params) const;
	auto start_exploratory_chat_chat();
	auto start_rigorous_chat_chat();
	auto start_general_chat();
	auto start_intuitive_chat();

	static godot::Ref<LLMChatParameters> get_exploratory_chat_params();
	static godot::Ref<LLMChatParameters> get_rigorous_chat_params();
	static godot::Ref<LLMChatParameters> get_general_chat_params();
	static godot::Ref<LLMChatParameters> get_intuitive_chat_params();

	llama_model* model() const { return model_.get(); }

private:
	std::unique_ptr<llama_model, decltype(&llama_model_free)> model_ { nullptr, &llama_model_free };
};
