#pragma once

#include "llm_chat.hpp"
#include "llm_chat_parameters.hpp"

#include <llama.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/weak_ref.hpp>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <string>
#include <thread>

class LLMChat;
class LLMChatParameters;
struct llama_model;
struct llama_context;

class LLM : public godot::RefCounted {
	GDCLASS(LLM, godot::RefCounted);

public:
	enum LoadStatus {
		MODEL_EMPTY = 0,
		MODEL_LOADING = 1,
		MODEL_READY = 2,
	};

	using llama_model_ptr = std::unique_ptr<llama_model, decltype(&llama_model_free)>;

	LLM() = default;
	~LLM() override;

	void load_model(const godot::String& path);
	void tick();
	godot::Ref<LLMChat> start_chat(const godot::Ref<LLMChatParameters>& params);
	godot::Ref<LLMChat> start_exploratory_chat_chat();
	godot::Ref<LLMChat> start_rigorous_chat_chat();
	godot::Ref<LLMChat> start_general_chat();
	godot::Ref<LLMChat> start_intuitive_chat();

	LoadStatus get_load_status() const;
	float get_load_progress() const;
	godot::String get_last_error() const;

	static godot::Ref<LLMChatParameters> get_exploratory_chat_params();
	static godot::Ref<LLMChatParameters> get_rigorous_chat_params();
	static godot::Ref<LLMChatParameters> get_general_chat_params();
	static godot::Ref<LLMChatParameters> get_intuitive_chat_params();

	llama_model* model() const { return model_.get(); }

protected:
	static void _bind_methods();

private:
	mutable std::shared_mutex mutex_;
	std::jthread background_thread_;
	llama_model_ptr model_ { nullptr, &llama_model_free };

	std::atomic<LoadStatus> load_status_ = MODEL_EMPTY;
	std::atomic<float> load_progress_ = 0;
	std::string last_error_;

	std::set<godot::ObjectID> chat_oids_;

	void load_model_routine(std::stop_token stoken, const std::string& path);
};
