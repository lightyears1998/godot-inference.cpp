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

	ADD_SIGNAL(MethodInfo("piece_generated", PropertyInfo(Variant::STRING, "content")));
	ADD_SIGNAL(MethodInfo("reply_generated", PropertyInfo(Variant::STRING, "content")));
}

LLMChat::LLMChat(const godot::Ref<LLMModel> &model, const godot::Ref<LLMChatParameters> &params)
	: model_(model), params_(params) {
	print_line(BOOST_CURRENT_FUNCTION);

	update_sampler();
}

LLMChat::~LLMChat() {
	print_line(BOOST_CURRENT_FUNCTION);

	cancel_generation();
}

void LLMChat::tick() {
	std::unique_lock lock(mutex_);

	if (!pending_outbound_piece_.empty()) {
		piece_generated(gstr(pending_outbound_piece_));
		pending_outbound_piece_.clear();
	}

	if (!pending_reply_.empty()) {
		record_message_locked("assistant", gstr(pending_reply_));
		reply_generated(gstr(pending_reply_));
		pending_reply_.clear();
	}
}

godot::Ref<LLMChatParameters> LLMChat::params() const { return params_; }

void LLMChat::job_routine(std::stop_token) {
	std::unique_lock<std::mutex> lock(mutex_);

	while (true) {
		cv_.wait(lock, stoken_, [&] { return !pending_inbound_messages_.empty(); });

		if (stoken_.stop_requested()) {
			break;
		}

		while (!stoken_.stop_requested() && !cancel_requested_) {
			throw std::exception("Not implemented"); // TODO
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
#if fasle
	auto vocab = llama_model_get_vocab(model_);
	auto n_ctx_train = llama_model_n_ctx_train(model_);
	constexpr const char* dry_sequence_breakers[] = {"\n", ":", "\"", "*"};
	constexpr size_t num_breakers = std::size(dry_sequence_breakers);
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_dry(vocab, n_ctx_train, 0.f, 1.75f, 2, -1,
		const_cast<const char**>(dry_sequence_breakers), // llama_sampler_init_dry won't modify `seq_breakers`, it is safe here.
		num_breakers));
#endif

	// TOP_N_SIGMA, disabled
#if fasle
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
#if false
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_xtc(0, 1, 0, -1));
#endif

	// TEMPERATURE
	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_temp(params_->temperature_));

	llama_sampler_chain_add(sampler_ptr, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}

void LLMChat::input_message_locked(const godot::StringName &role, const godot::String &message) {
	Ref msg = memnew(LLMChatMessage(role, message));
	pending_inbound_messages_.push(msg);
}

void LLMChat::record_message_locked(const godot::StringName &role, const godot::String &message) {
	messages_.emplace_back(ustr(role), ustr(message));
	godot_messages_.push_back(Ref(memnew(LLMChatMessage(role, message))));
}

void LLMChat::set_parameters(Ref<LLMChatParameters> params) {
	if (params.is_null()) return;

	params_ = params;
	update_sampler();
}

void LLMChat::say(const StringName& content) {
	std::lock_guard lock(mutex_);

	input_message_locked("user", content);
}

void LLMChat::remind(const StringName& content) {
	std::lock_guard lock(mutex_);

	input_message_locked("system", content);
}

void LLMChat::add_message(const StringName& role, const String& content) {
	std::lock_guard lock(mutex_);

	input_message_locked(role, content);
}

void LLMChat::clear_history() {
	std::lock_guard lock(mutex_);

	messages_.clear();
	godot_messages_.clear();
	last_reply_.clear();
}

void LLMChat::cancel_generation() {
	cancel_requested_ = true;
}

void LLMChat::request_reply() {
	std::unique_lock lock(mutex_);

	if (model_.is_null() || !ctx_) {
		print_error(BOOST_CURRENT_LOCATION.to_string().c_str());
		return;
	}

	cancel_requested_ = false;
	status_ = GENERATING;
	cv_.notify_all();
}

void LLMChat::reply_generated(godot::String content) {
	emit_signal("reply_generated", content);
}

void LLMChat::piece_generated(godot::String content) {
	emit_signal("piece_generated", content);
}

TypedArray<LLMChatMessage> LLMChat::history() const {
	std::unique_lock lock(mutex_);
	return godot_messages_;
}
