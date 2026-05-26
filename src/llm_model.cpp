#include "llm_model.hpp"

#include "llm_chat.hpp"
#include "llm_chat_parameters.hpp"

#include <boost/current_function.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void LLMModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("tick"), &LLMModel::tick);
	ClassDB::bind_method(D_METHOD("start_chat", "params"), &LLMModel::start_chat);
	ClassDB::bind_method(D_METHOD("start_exploratory_chat_chat"), &LLMModel::start_exploratory_chat_chat);
	ClassDB::bind_method(D_METHOD("start_rigorous_chat_chat"), &LLMModel::start_rigorous_chat_chat);
	ClassDB::bind_method(D_METHOD("start_general_chat"), &LLMModel::start_general_chat);
	ClassDB::bind_method(D_METHOD("start_intuitive_chat"), &LLMModel::start_intuitive_chat);

	ClassDB::bind_static_method("LLMModel", D_METHOD("get_exploratory_chat_params"), &LLMModel::get_exploratory_chat_params);
	ClassDB::bind_static_method("LLMModel", D_METHOD("get_rigorous_chat_params"), &LLMModel::get_rigorous_chat_params);
	ClassDB::bind_static_method("LLMModel", D_METHOD("get_general_chat_params"), &LLMModel::get_general_chat_params);
	ClassDB::bind_static_method("LLMModel", D_METHOD("get_intuitive_chat_params"), &LLMModel::get_intuitive_chat_params);
}

LLMModel::LLMModel(llama_model_ptr &&model)
	: model_(std::move(model)) {
	print_line(BOOST_CURRENT_FUNCTION);
}

LLMModel::~LLMModel() {
	print_line(BOOST_CURRENT_FUNCTION);

#if DEBUG_ENABLED
	for (auto it = chat_oids_.begin(); it != chat_oids_.end(); ++it) {
		if (const auto chat = dynamic_cast<LLMChat *>(ObjectDB::get_instance(*it)); chat != nullptr) {
			print_error("leaked chat: %lld", *it);
		}
	}
#endif
}

void LLMModel::tick() {
	for (auto it = chat_oids_.begin(); it != chat_oids_.end(); ) {
		auto chat = dynamic_cast<LLMChat*>(ObjectDB::get_instance(*it));
		if (chat == nullptr) {
			it = chat_oids_.erase(it);
		} else {
			chat->tick();
			++it;
		}
	}
}

// TODO don't block ui
godot::Ref<LLMChat> LLMModel::start_chat(const godot::Ref<LLMChatParameters>& params) {
	if (!model_) {
		return {};
	}

	Ref<LLMChat> chat = memnew(LLMChat(this, params));
	chat_oids_.insert(ObjectID(chat->get_instance_id()));
	return chat;
}

Ref<LLMChat> LLMModel::start_exploratory_chat_chat() {
	return start_chat(get_exploratory_chat_params());
}

Ref<LLMChat> LLMModel::start_rigorous_chat_chat() {
	return start_chat(get_exploratory_chat_params());
}

Ref<LLMChat> LLMModel::start_general_chat() {
	return start_chat(get_exploratory_chat_params());
}

Ref<LLMChat> LLMModel::start_intuitive_chat() {
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
