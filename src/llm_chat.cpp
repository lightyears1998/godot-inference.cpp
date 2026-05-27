#include "llm_chat.hpp"

#include "llm_chat_message.hpp"
#include "llm_chat_parameters.hpp"
#include "llm_model.hpp"
#include "utils.hpp"

#include <llama.h>
#include <boost/assert/source_location.hpp>
#include <boost/current_function.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <shared_mutex>

using namespace godot;

void LLMChat::_bind_methods() {
	BIND_ENUM_CONSTANT(IDLE);
	BIND_ENUM_CONSTANT(GENERATING);

	ClassDB::bind_method(D_METHOD("say", "content"), &LLMChat::say);
	ClassDB::bind_method(D_METHOD("remind", "content"), &LLMChat::remind);
	ClassDB::bind_method(D_METHOD("add_message", "role", "message"), &LLMChat::add_message);
	ClassDB::bind_method(D_METHOD("request_reply"), &LLMChat::request_reply);
	ClassDB::bind_method(D_METHOD("cancel_generation"), &LLMChat::cancel_generation);
	ClassDB::bind_method(D_METHOD("clear_history"), &LLMChat::clear_history);

	ClassDB::bind_method(D_METHOD("get_params"), &LLMChat::params);
	ClassDB::bind_method(D_METHOD("set_parameters", "params"), &LLMChat::set_parameters);
	ClassDB::bind_method(D_METHOD("get_history"), &LLMChat::history);
	ClassDB::bind_method(D_METHOD("get_last_reply"), &LLMChat::last_reply);
	ClassDB::bind_method(D_METHOD("is_valid"), &LLMChat::is_valid);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "params"), "set_parameters", "get_params");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "history"), "", "get_history");

	ADD_SIGNAL(MethodInfo("piece_generated", PropertyInfo(Variant::STRING, "content")));
	ADD_SIGNAL(MethodInfo("reply_generated", PropertyInfo(Variant::STRING, "content")));
}

LLMChat::LLMChat(const godot::Ref<LLMModel> &model, const godot::Ref<LLMChatParameters> &params)
	: model_(model), params_(params) {
	print_line(BOOST_CURRENT_FUNCTION);

	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = params->context_length_;
	ctx_params.n_batch = 2048;

	llama_context* ctx = llama_init_from_model(model_->model(), ctx_params);
	if (!ctx) {
		print_error("Failed to create context.");
		return;
	}
	ctx_ = { ctx, &llama_free };

	update_sampler();

	job_thread_ = std::jthread([&](std::stop_token stoken) {
		job_routine(std::move(stoken));
	});
}

LLMChat::~LLMChat() {
	print_line(BOOST_CURRENT_FUNCTION);

	cancel_generation();
	job_thread_.request_stop();
	cv_.notify_all();
}

void LLMChat::tick() {
	std::string piece_to_emit;
	std::string reply_to_emit;

	{
		std::lock_guard lock(mutex_);

		if (!pending_outbound_piece_.empty()) {
			piece_to_emit = std::move(pending_outbound_piece_);
			pending_outbound_piece_.clear();
		}

		if (!pending_reply_.empty()) {
			record_message_locked("assistant", gstr(pending_reply_));
			reply_to_emit = pending_reply_;
			last_reply_ = pending_reply_;
			pending_reply_.clear();
		}
	}

	if (!piece_to_emit.empty()) {
		piece_generated(gstr(piece_to_emit));
	}

	if (!reply_to_emit.empty()) {
		reply_generated(gstr(reply_to_emit));
	}
}

Ref<LLMChatParameters> LLMChat::params() const {
	std::lock_guard lock(mutex_);

	return params_;
}

void LLMChat::job_routine(std::stop_token stoken) {
	auto vocab = llama_model_get_vocab(model_->model());

	while (true) {
		{
			std::unique_lock<std::mutex> lock(mutex_);
			cv_.wait(lock, stoken, [&] {
				return stoken.stop_requested() || cancel_requested_.load() || !pending_inbound_messages_.empty();
			});

			if (stoken.stop_requested()) break;

			if (cancel_requested_.load()) {
				cancel_requested_ = false;
				status_ = IDLE;
				continue;
			}
		}

		std::vector<godot::Ref<LLMChatMessage>> messages_to_process;
		{
			std::lock_guard lock(mutex_);
			while (!pending_inbound_messages_.empty()) {
				messages_to_process.push_back(pending_inbound_messages_.front());
				pending_inbound_messages_.pop_front();
			}
		}

		if (messages_to_process.empty()) continue;

		std::string prompt;
		{
			std::lock_guard lock(mutex_);
			for (const auto &msg : messages_to_process) {
				prompt += "<|im_start|>";
				prompt += ustr(msg->role_);
				prompt += "\n";
				prompt += ustr(msg->message_);
				prompt += "<|im_end|>\n";
			}
		}
		prompt += "<|im_start|>assistant\n";

		{
			std::lock_guard lock(mutex_);

			if (params_->thinking_enabled_) {
				prompt += "<think>\n";
			} else {
				prompt += "<think>\n\n</think>\n\n";
			}
		}

		const bool add_special = [&] {
			std::lock_guard lock(mutex_);
			return llama_memory_seq_pos_max(llama_get_memory(ctx_.get()), 0) == -1;
		}();

		auto token_len = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, add_special, true);
		std::vector<llama_token> tokens(token_len, 0);
		auto ret = llama_tokenize(vocab, prompt.c_str(), prompt.size(), tokens.data(), tokens.size(), add_special, true);
		if (ret < 0 || ret != token_len) {
			print_error(BOOST_CURRENT_LOCATION.to_string().c_str());
			std::lock_guard lock(mutex_);
			status_ = IDLE;
			continue;
		}

		if (stoken.stop_requested()) break;

		std::string response;
		llama_token new_token_id;
		auto batch = llama_batch_get_one(tokens.data(), tokens.size());

		while (!stoken.stop_requested() && !cancel_requested_.load()) {
			{
				std::lock_guard lock(mutex_);
				int n_ctx = llama_n_ctx(ctx_.get());
				int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx_.get()), 0) + 1;
				if (n_ctx_used + batch.n_tokens > n_ctx) {
					print_error("context size exceeded!");
					break;
				}
			}

			{
				std::lock_guard lock(mutex_);
				ret = llama_decode(ctx_.get(), batch);
			}

			if (ret != 0) {
				print_error("failed to decode, ret: ", ret);
				break;
			}

			llama_token sampled_token;
			{
				std::lock_guard lock(mutex_);
				sampled_token = llama_sampler_sample(sampler_.get(), ctx_.get(), -1);
			}

			if (llama_vocab_is_eog(vocab, sampled_token)) {
				break;
			}

			char buf[256];
			int n = llama_token_to_piece(vocab, sampled_token, buf, sizeof(buf), 0, true);
			if (n < 0) {
				print_error("failed to convert token to piece");
				break;
			}
			std::string piece(buf, n);

			{
				std::lock_guard lock(mutex_);
				pending_outbound_piece_ += piece;
			}

			response += piece;
			new_token_id = sampled_token;
			batch = llama_batch_get_one(&new_token_id, 1);
		}

		{
			std::lock_guard lock(mutex_);
			pending_reply_ = response;
			status_ = IDLE;
		}
	}
}

void LLMChat::update_sampler() {
	std::unique_lock lock(mutex_);

	if (sampler_) {
		sampler_.reset();
	}

	auto sampler_params = llama_sampler_chain_default_params();

	sampler_ = std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)>(
		llama_sampler_chain_init(sampler_params), llama_sampler_free);
	auto sampler_ptr = sampler_.get();

	// Keep the same order with llama.cpp tools.
	// ref: llama.cpp/src/common/common.h::common_params_sampling::samplers

	// PENALTIES
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_penalties(64, params_->repeat_penalty_, 0, params_->presence_penalty_));

	// DRY, disabled
#if 0
	auto vocab = llama_model_get_vocab(model_);
	auto n_ctx_train = llama_model_n_ctx_train(model_);
	constexpr const char* dry_sequence_breakers[] = {"\n", ":", "\"", "*"};
	constexpr size_t num_breakers = std::size(dry_sequence_breakers);
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_dry(vocab, n_ctx_train, 0.f, 1.75f, 2, -1,
		const_cast<const char**>(dry_sequence_breakers),
		num_breakers));
#endif

	// TOP_N_SIGMA, disabled
#if 0
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_top_n_sigma(-1.00f));
#endif

	// TOP_K
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_top_k(params_->top_k_));

	// TYPICAL_P, disabled
#if false
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_typical(1, 0));
#endif

	// TOP_P
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_top_p(params_->top_p_, 0));

	// MIN_P
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_min_p(params_->min_p_, 0));

	// XTC, disabled
#if 0
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_xtc(0, 1, 0, -1));
#endif

	// TEMPERATURE
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_temp(params_->temperature_));

	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}

void LLMChat::input_message_locked(const godot::StringName &role, const godot::String &message) {
	Ref msg = memnew(LLMChatMessage(role, message));
	pending_inbound_messages_.emplace_back(msg);
	record_message_locked(role, message);
}

void LLMChat::record_message_locked(const godot::StringName &role, const godot::String &message) {
	messages_.emplace_back(ustr(role), ustr(message));
	godot_messages_.push_back(Ref(memnew(LLMChatMessage(role, message))));
}

void LLMChat::set_parameters(Ref<LLMChatParameters> params) {
	if (params.is_null()) return;

	std::lock_guard lock(mutex_);

	if (status_ == GENERATING) {
		print_error("Cannot set parameters while generating.");
		return;
	}

	params_ = params;
	update_sampler();
}

void LLMChat::say(const String& content) {
	std::lock_guard lock(mutex_);

	input_message_locked("user", content);
}

void LLMChat::remind(const String& content) {
	std::lock_guard lock(mutex_);

	input_message_locked("system", content);
}

void LLMChat::add_message(const StringName& role, const String& content) {
	std::lock_guard lock(mutex_);

	input_message_locked(role, content);
}

void LLMChat::clear_history() {
	std::lock_guard lock(mutex_);

	pending_inbound_messages_.clear();
	pending_outbound_piece_.clear();
	messages_.clear();
	godot_messages_.clear();
	last_reply_.clear();

	if (ctx_) {
		llama_memory_clear(llama_get_memory(ctx_.get()), true);
	}
}

void LLMChat::cancel_generation() {
	cancel_requested_ = true;
	cv_.notify_all();
}

void LLMChat::request_reply() {
	std::unique_lock lock(mutex_);

	if (!ctx_) {
		print_error(BOOST_CURRENT_LOCATION.to_string().c_str());
		return;
	}

	cancel_requested_ = false;
	status_ = GENERATING;
	cv_.notify_all();
}

void LLMChat::reply_generated(String content) {
	emit_signal("reply_generated", content);
}

void LLMChat::piece_generated(String content) {
	emit_signal("piece_generated", content);
}

String LLMChat::last_reply() const {
	std::lock_guard lock(mutex_);

	return gstr(last_reply_);
}

TypedArray<LLMChatMessage> LLMChat::history() const {
	std::lock_guard lock(mutex_);

	return godot_messages_;
}
