#include "llm_engine.hpp"

#include "constants.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "llm_model.hpp"
#include "utils.hpp"

#include <llama.h>
#include <boost/current_function.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <iostream>

using namespace godot;

void LLMEngine::_bind_methods() {
	BIND_ENUM_CONSTANT(MODEL_EMPTY);
	BIND_ENUM_CONSTANT(MODEL_LOADING);
	BIND_ENUM_CONSTANT(MODEL_READY);

	ClassDB::bind_static_method("LLMEngine", D_METHOD("tick"), &LLMEngine::tick);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("request_load_model", "path"), &LLMEngine::request_load_model, DEFVAL(""));
	ClassDB::bind_static_method("LLMEngine", D_METHOD("unload_model"), &LLMEngine::unload_model);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model_load_status"), &LLMEngine::model_load_status);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model_load_progress"), &LLMEngine::model_load_progress);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model"), &LLMEngine::model);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_last_error"), &LLMEngine::last_error);
}

void LLMEngine::init_backend() {
	print_line(BOOST_CURRENT_FUNCTION);

	std::unique_lock lock(mutex_);

	llama_log_set([](enum ggml_log_level level, const char* text, void*) {
		if (level < GGML_LOG_LEVEL_WARN) {
			return;
		}

		String log(text);
		log.resize(log.length() - 1);
		print_line(log);
	}, nullptr);

	llama_backend_init();
}

void LLMEngine::tick() {
	std::shared_lock lock(mutex_);
	if (model_.is_valid()) {
		model_->tick();
	}
}

void LLMEngine::free_backend() {
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

void LLMEngine::request_load_model(String path) {
	if (path.is_empty()) {
		path = String(ProjectSettings::get_singleton()->get_setting(SETTINGS_KEY_MODEL_PATH, "model.gguf"));
	}

	// Not a strict check, but should be enough for now. 
	if (model_status_ == MODEL_LOADING) {
		return;
	}
	model_status_ = MODEL_LOADING;

	load_progress_ = 0.0f;

	background_thread_ = std::jthread(&LLMEngine::load_model, ustr(path));
}

void LLMEngine::unload_model() {
	std::unique_lock lock(mutex_);
	model_.unref();
}

void LLMEngine::load_model(std::stop_token stoken, std::string path) {
	{	
		std::unique_lock lock(mutex_);
		model_status_ = MODEL_LOADING;
		load_progress_ = 0.0f;
	}

	try {
		const llama_progress_callback progress_cb = [](float progress, void* stoken) -> bool {
			load_progress_ = progress;
			return !static_cast<std::stop_token*>(stoken)->stop_requested();
		};

		llama_model_params model_params = llama_model_default_params();
		model_params.n_gpu_layers = -1;
		model_params.progress_callback = progress_cb;
		model_params.progress_callback_user_data = &stoken;

		LLMModel::llama_model_ptr model = { llama_model_load_from_file(path.c_str(), model_params), &llama_model_free };
		if (!model) {
			throw std::runtime_error("Unable to load model");
		}

		std::unique_lock lock(mutex_);
		model_ = Ref<LLMModel>(memnew(LLMModel(std::move(model))));
		model_status_ = MODEL_READY;
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

LLMEngine::ModelStatus LLMEngine::model_load_status() {
	return model_status_;
}

float LLMEngine::model_load_progress() {
	return load_progress_;
}

Ref<LLMModel> LLMEngine::model() {
	std::shared_lock lock(mutex_);
	return model_;
}

String LLMEngine::last_error() {
	std::shared_lock lock(mutex_);
	return String(last_error_.c_str());
}
