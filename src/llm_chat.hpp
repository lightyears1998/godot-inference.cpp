#pragma once

#include "llm_chat_message.hpp"
#include "llm_model.hpp"

#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <llama.h>
#include <godot_cpp/classes/ref_counted.hpp>

class LLMChatParameters;

class LLMChat : public godot::RefCounted {
	GDCLASS(LLMChat, godot::RefCounted)

protected:
	static void _bind_methods();

public:
	enum ReplyGenerationStatus {
		IDLE = 0,
		GENERATING = 1,
	};

public:
	LLMChat() = default;
	LLMChat(llama_model* model, llama_context* ctx, const godot::Ref<LLMChatParameters> &params);
	~LLMChat() override;

	godot::Ref<LLMChatParameters> params() const;
	void set_parameters(godot::Ref<LLMChatParameters> params);

	void say(const godot::String& content);
	void remind(const godot::String& content);
	void add_message(const godot::String &role, const godot::String &message);
	godot::String request_reply();
	void reply_generated(godot::String content);

	ReplyGenerationStatus generation_status() const { return status_.load(); }
	void cancel_generation();
	void piece_generated(godot::String content);

	godot::String last_reply() const { return godot::String(last_reply_.c_str()); }
	godot::String last_error() const;

	godot::TypedArray<LLMChatMessage> history() const;
	void clear_history();

private:
	mutable std::mutex mutex_;
	llama_model* model_ = nullptr;
	llama_context* ctx_ = nullptr;
	godot::Ref<LLMChatParameters> params_;
	std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)> sampler_ { nullptr, &llama_sampler_free };
	std::vector<llama_chat_message> messages_;
	std::string last_reply_;
	std::atomic<ReplyGenerationStatus> status_ { IDLE };
	std::atomic<bool> cancel_requested_ { false };

	void update_sampler();
};

VARIANT_ENUM_CAST(LLMChat::ReplyGenerationStatus);
