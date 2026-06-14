#include "llm.hpp"

#include "llm_chat.hpp"
#include "llm_chat_parameters.hpp"
#include "utils.hpp"

#include <boost/current_function.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void LLM::_bind_methods() {
	BIND_ENUM_CONSTANT(MODEL_EMPTY);
	BIND_ENUM_CONSTANT(MODEL_LOADING);
	BIND_ENUM_CONSTANT(MODEL_READY);

	ClassDB::bind_method(D_METHOD("load_model", "path"), &LLM::load_model);
	ClassDB::bind_method(D_METHOD("tick"), &LLM::tick);
	ClassDB::bind_method(D_METHOD("start_chat", "params"), &LLM::start_chat);
	ClassDB::bind_method(D_METHOD("start_exploratory_chat_chat"), &LLM::start_exploratory_chat_chat);
	ClassDB::bind_method(D_METHOD("start_rigorous_chat_chat"), &LLM::start_rigorous_chat_chat);
	ClassDB::bind_method(D_METHOD("start_general_chat"), &LLM::start_general_chat);
	ClassDB::bind_method(D_METHOD("start_intuitive_chat"), &LLM::start_intuitive_chat);
	ClassDB::bind_method(D_METHOD("get_load_status"), &LLM::get_load_status);
	ClassDB::bind_method(D_METHOD("get_load_progress"), &LLM::get_load_progress);
	ClassDB::bind_method(D_METHOD("get_last_error"), &LLM::get_last_error);

	ClassDB::bind_static_method("LLM", D_METHOD("get_exploratory_chat_params"), &LLM::get_exploratory_chat_params);
	ClassDB::bind_static_method("LLM", D_METHOD("get_rigorous_chat_params"), &LLM::get_rigorous_chat_params);
	ClassDB::bind_static_method("LLM", D_METHOD("get_general_chat_params"), &LLM::get_general_chat_params);
	ClassDB::bind_static_method("LLM", D_METHOD("get_intuitive_chat_params"), &LLM::get_intuitive_chat_params);

	ADD_SIGNAL(MethodInfo("model_loaded"));
}

LLM::~LLM() {
	print_line(BOOST_CURRENT_FUNCTION);

	if (background_thread_.joinable()) {
		background_thread_.request_stop();
		background_thread_.join();
	}

#if DEBUG_ENABLED
	for (auto it = chat_oids_.begin(); it != chat_oids_.end(); ++it) {
		if (const auto chat = dynamic_cast<LLMChat *>(ObjectDB::get_instance(*it)); chat != nullptr) {
			print_error("leaked chat: %lld", *it);
		}
	}
#endif
}

void LLM::load_model(const godot::String& path) {
	if (path.is_empty()) {
		print_error("Model path cannot be empty.");
		return;
	}

	if (load_status_ == MODEL_LOADING) {
		return;
	}

	load_status_ = MODEL_LOADING;
	load_progress_ = 0.0f;

	background_thread_ = std::jthread([=, this](std::stop_token stoken) {
		load_model_routine(std::move(stoken), ustr(path));
	});
}

void LLM::load_model_routine(std::stop_token stoken, const std::string& path) {
	{	
		std::unique_lock lock(mutex_);
		load_status_ = MODEL_LOADING;
		load_progress_ = 0.0f;
	}

	try {
		struct {
			decltype(load_progress_)* load_progress;
			decltype(stoken)* stoken;
		} llama_progress_callback_context { .load_progress = &load_progress_, .stoken = &stoken };

		constexpr llama_progress_callback progress_cb = [](float progress, void* user_data) -> bool {
			const auto ctx = static_cast<decltype(llama_progress_callback_context)*>(user_data);

			ctx->load_progress->store(progress);
			return !ctx->stoken->stop_requested();
		};

		llama_model_params model_params = llama_model_default_params();
		model_params.n_gpu_layers = -1;
		model_params.progress_callback = progress_cb;
		model_params.progress_callback_user_data = &llama_progress_callback_context;

		llama_model_ptr model = { llama_model_load_from_file(path.c_str(), model_params), &llama_model_free };
		if (!model) {
			throw std::runtime_error("Unable to load model");
		}

		{
			std::unique_lock lock(mutex_);
			model_ = std::move(model);
			load_status_ = MODEL_READY;
		}

		call_deferred("emit_signal", "model_loaded");
	} catch (const std::exception& e) {
		std::unique_lock lock(mutex_);
		last_error_ = e.what();
		load_status_ = MODEL_EMPTY;
		load_progress_ = 0;
	} catch (...) {
		std::unique_lock lock(mutex_);
		last_error_ = "Unknown error";
		load_status_ = MODEL_EMPTY;
		load_progress_ = 0;
	}
}

LLM::LoadStatus LLM::get_load_status() const {
	std::shared_lock lock(mutex_);
	return load_status_;
}

float LLM::get_load_progress() const {
	std::shared_lock lock(mutex_);
	return load_progress_;
}

godot::String LLM::get_last_error() const {
	std::shared_lock lock(mutex_);
	return String(last_error_.c_str());
}

void LLM::tick() {
	for (auto it = chat_oids_.begin(); it != chat_oids_.end(); ) {
		auto chat = static_cast<LLMChat*>(ObjectDB::get_instance(*it));
		if (chat == nullptr) {
			it = chat_oids_.erase(it);
		} else {
			chat->tick();
			++it;
		}
	}
}

Ref<LLMChat> LLM::start_chat(const godot::Ref<LLMChatParameters>& params) {
	if (!model_) {
		return {};
	}

	Ref<LLMChat> chat = memnew(LLMChat(this, params));
	chat_oids_.insert(ObjectID(chat->get_instance_id()));
	return chat;
}

Ref<LLMChat> LLM::start_exploratory_chat_chat() {
	return start_chat(get_exploratory_chat_params());
}

Ref<LLMChat> LLM::start_rigorous_chat_chat() {
	return start_chat(get_rigorous_chat_params());
}

Ref<LLMChat> LLM::start_general_chat() {
	return start_chat(get_general_chat_params());
}

Ref<LLMChat> LLM::start_intuitive_chat() {
	return start_chat(get_intuitive_chat_params());
}

Ref<LLMChatParameters> LLM::get_exploratory_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->thinking_enabled_ = true;
	ret->temperature_ = 1;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}

Ref<LLMChatParameters> LLM::get_rigorous_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->thinking_enabled_ = true;
	ret->temperature_ = 0.6;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 0;
	ret->repeat_penalty_ = 1;
	return ret;
}

Ref<LLMChatParameters> LLM::get_general_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->thinking_enabled_ = false;
	ret->temperature_ = 0.7;
	ret->top_p_ = 0.8;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}

Ref<LLMChatParameters> LLM::get_intuitive_chat_params() {
	auto ret = memnew(LLMChatParameters);
	ret->thinking_enabled_ = false;
	ret->temperature_ = 1;
	ret->top_p_ = 0.95;
	ret->top_k_ = 20;
	ret->min_p_ = 0;
	ret->presence_penalty_ = 1.5;
	ret->repeat_penalty_ = 1;
	return ret;
}

VARIANT_ENUM_CAST(LLM::LoadStatus);
