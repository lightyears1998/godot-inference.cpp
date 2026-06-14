#pragma once

#include "core/chat_message.hpp"
#include "llm.hpp"
#include "llm_chat_message.hpp"

#include <llama.h>
#include <atomic>
#include <condition_variable>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class LLM;
class LLMChatParameters;

// TODO optimize data-flow
class LLMChat : public godot::RefCounted {
	GDCLASS(LLMChat, RefCounted)

public:
	enum ReplyGenerationStatus {
		IDLE = 0,
		GENERATING = 1,
	};

	LLMChat() = default;
	LLMChat(const godot::Ref<LLM> &model, const godot::Ref<LLMChatParameters> &params);
	~LLMChat() override;

	// props
	bool is_valid() const { return ctx_ != nullptr; }
	godot::Ref<LLMChatParameters> params() const;
	void set_parameters(godot::Ref<LLMChatParameters> params);
	ReplyGenerationStatus generation_status() const { return status_.load(); }
	godot::String last_reply() const;
	godot::TypedArray<LLMChatMessage> history() const;

	// hook
	void tick();

	// op
	void say(const godot::String& content);
	void remind(const godot::String& content);
	void add_message(const godot::StringName &role, const godot::String &message);
	void request_reply();
	void cancel_generation();
	void clear_history();

	// signals
	void piece_generated(godot::String content);
	void reply_generated(godot::String content);

protected:
	static void _bind_methods();

private:
	// mutex
	mutable std::mutex mutex_;
	std::condition_variable_any cv_;
	std::atomic<bool> generation_requested_;
	std::atomic<bool> cancel_requested_;

	// ref
	godot::Ref<LLM> model_ = nullptr;
	godot::Ref<LLMChatParameters> params_;
	std::unique_ptr<llama_context, decltype(&llama_free)> ctx_ = { nullptr, &llama_free };
	std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)> sampler_ { nullptr, &llama_sampler_free };
	std::jthread job_thread_;

	// inbound
	std::deque<godot::Ref<LLMChatMessage>> pending_inbound_messages_;

	// outbound
	std::string pending_outbound_piece_;
	std::string pending_reply_;

	// props
	std::vector<core::ChatMessage> messages_;
	godot::TypedArray<LLMChatMessage> godot_messages_;
	std::atomic<ReplyGenerationStatus> status_ { IDLE };
	std::string last_reply_;

	void job_routine(std::stop_token stoken);
	void update_sampler();
	void input_message_locked(const godot::StringName &role, const godot::String &message);
	void record_message_locked(const godot::StringName &role, const godot::String &message);
};

VARIANT_ENUM_CAST(LLMChat::ReplyGenerationStatus);
