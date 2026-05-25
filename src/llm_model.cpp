#include "llm_model.hpp"

#include "llm_chat.hpp"
#include "llm_chat_parameters.hpp"

#include <godot_cpp/core/class_db.hpp>

void LLMModel::_bind_methods() {
	using namespace godot;
	ClassDB::bind_method(D_METHOD("start_chat", "params"), &LLMModel::start_chat);
}

LLMModel::LLMModel(llama_model* model)
	: model_(model, llama_model_free) {
}

godot::Ref<LLMChat> LLMModel::start_chat(const godot::Ref<LLMChatParameters>& params) const {
	if (!model_) {
		return {};
	}

	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = params->context_length_;
	ctx_params.n_batch = 2048;

	llama_context* ctx = llama_init_from_model(model_.get(), ctx_params);
	if (!ctx) {
		return {};
	}

	return {memnew(LLMChat(model_.get(), ctx, params))};
}

auto LLMModel::start_exploratory_chat_chat() {
	return start_chat(get_exploratory_chat_params());
}

auto LLMModel::start_rigorous_chat_chat() {
	return start_chat(get_exploratory_chat_params());
}

auto LLMModel::start_general_chat() {
	return start_chat(get_exploratory_chat_params());
}

auto LLMModel::start_intuitive_chat() {
	return start_chat(get_exploratory_chat_params());
}

godot::Ref<LLMChatParameters> LLMModel::get_exploratory_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->temperature_ = 1;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}

godot::Ref<LLMChatParameters> LLMModel::get_rigorous_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->temperature_ = 0.6;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 0;
	ret->repeat_penalty_ = 1;
	return ret;
}

godot::Ref<LLMChatParameters> LLMModel::get_general_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->temperature_ = 0.7;
	ret->top_p_ = 0.8;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}

godot::Ref<LLMChatParameters> LLMModel::get_intuitive_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->temperature_ = 1;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}
