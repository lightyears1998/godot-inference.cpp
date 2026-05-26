#pragma once

#include "llm_chat.hpp"

#include <llama.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/weak_ref.hpp>
#include <memory>

class LLMChat;
class LLMChatParameters;
struct llama_model;
struct llama_context;

class LLMModel : public godot::RefCounted {
	GDCLASS(LLMModel, godot::RefCounted);

public:
	using llama_model_ptr = std::unique_ptr<llama_model, decltype(&llama_model_free)>;

	LLMModel() = default;
	LLMModel(llama_model_ptr&& model);
	~LLMModel() override;

	void tick();
	godot::Ref<LLMChat> start_chat(const godot::Ref<LLMChatParameters>& params);
	godot::Ref<LLMChat> start_exploratory_chat_chat();
	godot::Ref<LLMChat> start_rigorous_chat_chat();
	godot::Ref<LLMChat> start_general_chat();
	godot::Ref<LLMChat> start_intuitive_chat();

	static godot::Ref<LLMChatParameters> get_exploratory_chat_params();
	static godot::Ref<LLMChatParameters> get_rigorous_chat_params();
	static godot::Ref<LLMChatParameters> get_general_chat_params();
	static godot::Ref<LLMChatParameters> get_intuitive_chat_params();

	llama_model* model() const { return model_.get(); }

protected:
	static void _bind_methods();

private:
	llama_model_ptr model_ { nullptr, &llama_model_free };
	std::set<godot::ObjectID> chat_oids_;
};
