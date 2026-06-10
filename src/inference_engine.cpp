#include "inference_engine.hpp"

#include "asr_model.hpp"
#include "constants.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "llm_model.hpp"
#include "utils.hpp"

#include <llama.h>
#include <whisper.h>
#include <boost/current_function.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <iostream>
#include <utility>

using namespace godot;

void InferenceEngine::_bind_methods() {
	BIND_ENUM_CONSTANT(MODEL_EMPTY);
	BIND_ENUM_CONSTANT(MODEL_LOADING);
	BIND_ENUM_CONSTANT(MODEL_READY);

	ClassDB::bind_method(D_METHOD("tick"), &InferenceEngine::tick);
	ClassDB::bind_method(D_METHOD("request_load_model", "path"), &InferenceEngine::request_load_model);
	ClassDB::bind_method(D_METHOD("request_load_asr_model", "path"), &InferenceEngine::request_load_asr_model);
	ClassDB::bind_method(D_METHOD("unload_model"), &InferenceEngine::unload_model);
	ClassDB::bind_method(D_METHOD("get_model_load_status"), &InferenceEngine::model_load_status);
	ClassDB::bind_method(D_METHOD("get_model_load_progress"), &InferenceEngine::model_load_progress);
	ClassDB::bind_method(D_METHOD("get_model"), &InferenceEngine::model);
	ClassDB::bind_method(D_METHOD("get_last_error"), &InferenceEngine::last_error);

	ADD_SIGNAL(MethodInfo("model_loaded", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::OBJECT, "model", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "LLMModel")));
}

void InferenceEngine::init_backend() {
	print_line(BOOST_CURRENT_FUNCTION);

	std::unique_lock lock(mutex_);

	constexpr auto log_handler = [](enum ggml_log_level level, const char* text, void*) {
		if (level < GGML_LOG_LEVEL_WARN) {
			return;
		}

		print_line(String(text).rstrip("\n"));
	};

	llama_log_set(log_handler, nullptr);
	whisper_log_set(log_handler, nullptr);

	llama_backend_init();
}

void InferenceEngine::tick() {
	std::shared_lock lock(mutex_);
	if (model_.is_valid()) {
		model_->tick();
	}
}

void InferenceEngine::free_backend() {
	print_line(BOOST_CURRENT_FUNCTION);

	std::unique_lock lock(mutex_);

	if (background_thread_.joinable()) {
		background_thread_.request_stop();
		lock.unlock();
		background_thread_.join();
	}

	if (!lock.owns_lock()) {
		lock.lock();
	}
	model_.unref(); // TODO graceful shutdown
	llama_backend_free();
}

void InferenceEngine::request_load_model(const String& path) {
	if (path.is_empty()) {
		print_error("Model path cannot be empty.");
		return;
	}

	// Not a strict check, but should be enough for now. 
	if (model_status_ == MODEL_LOADING) {
		return;
	}
	model_status_ = MODEL_LOADING;

	load_progress_ = 0.0f;

	background_thread_ = std::jthread([=, this](std::stop_token stoken) {
		load_model(std::move(stoken), ustr(path));
	});
}

Ref<ASRModel> InferenceEngine::request_load_asr_model(const godot::String &path) {
	Ref<ASRModel> model = memnew(ASRModel(path));
	return model;
}

void InferenceEngine::unload_model() {
	std::unique_lock lock(mutex_);
	model_.unref();
}

void InferenceEngine::load_model(std::stop_token stoken, const std::string& path) {
	{	
		std::unique_lock lock(mutex_);
		model_status_ = MODEL_LOADING;
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

		LLMModel::llama_model_ptr model = { llama_model_load_from_file(path.c_str(), model_params), &llama_model_free };
		if (!model) {
			throw std::runtime_error("Unable to load model");
		}

		std::unique_lock lock(mutex_);
		model_ = Ref<LLMModel>(memnew(LLMModel(std::move(model))));
		model_status_ = MODEL_READY;
		model_loaded(gstr(path), model_);
	} catch (const std::exception& e) {
		std::unique_lock lock(mutex_);
		last_error_ = e.what();
		model_status_ = MODEL_EMPTY;
		load_progress_ = 0;
	} catch (...) {
		std::unique_lock lock(mutex_);
		last_error_ = "Unknown error";
		model_status_ = MODEL_EMPTY;
		load_progress_ = 0;
	}
}

void InferenceEngine::model_loaded(const String& model_path, const Ref<LLMModel>& model) {
	call_deferred("emit_signal", "model_loaded", model_path, model);
}

InferenceEngine::ModelStatus InferenceEngine::model_load_status() {
	return model_status_;
}

float InferenceEngine::model_load_progress() {
	return load_progress_;
}

Ref<LLMModel> InferenceEngine::model() {
	std::shared_lock lock(mutex_);
	return model_;
}

String InferenceEngine::last_error() {
	std::shared_lock lock(mutex_);
	return String(last_error_.c_str());
}
