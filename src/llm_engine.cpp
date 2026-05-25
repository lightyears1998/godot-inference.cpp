#include "llm_engine.hpp"

#include "constants.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "llm_model.hpp"
#include "utils.hpp"

#include <llama.h>
#include <godot_cpp/core/class_db.hpp>
#include <iostream>

using namespace godot;

void LLMEngine::_bind_methods() {
	BIND_ENUM_CONSTANT(MODEL_EMPTY);
	BIND_ENUM_CONSTANT(MODEL_LOADING);
	BIND_ENUM_CONSTANT(MODEL_READY);

	ClassDB::bind_static_method("LLMEngine", D_METHOD("init_backend"), &LLMEngine::init_backend);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("tick"), &LLMEngine::tick);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("free_backend"), &LLMEngine::free_backend);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("request_load_model", "path"), &LLMEngine::request_load_model, DEFVAL(""));
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model_load_status"), &LLMEngine::model_load_status);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model_load_progress"), &LLMEngine::model_load_progress);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_model"), &LLMEngine::model);
	ClassDB::bind_static_method("LLMEngine", D_METHOD("get_last_error"), &LLMEngine::last_error);
}

void LLMEngine::init_backend() {
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
}

void LLMEngine::free_backend() {
	std::unique_lock lock(mutex_);
	model_.reset();
	backend_thread_.reset();
	llama_backend_free();
}

void LLMEngine::request_load_model(String path) {
	if (path.is_empty()) {
		path = String(ProjectSettings::get_singleton()->get_setting(SETTINGS_KEY_MODEL_PATH, "model.gguf"));
	}

	std::unique_lock lock(mutex_);
	if (model_status_.load() != MODEL_EMPTY) {
		return;
	}

	model_status_ = MODEL_LOADING;
	backend_thread_ = std::jthread(&LLMEngine::load_model, ustr(path));
}

void LLMEngine::load_model(std::string path) {
	model_status_.store(MODEL_LOADING);
	load_progress_.store(0.0f);

	try {
		const llama_progress_callback progress_cb = [](float progress, void* _) {
			load_progress_ = progress;
			return true;
		};

		llama_model_params model_params = llama_model_default_params();
		model_params.n_gpu_layers = -1;
		model_params.progress_callback = progress_cb;

		llama_model* model = llama_model_load_from_file(path.c_str(), model_params);
		if (!model) {
			throw std::runtime_error("Unable to load model");
		}

		std::unique_lock lock(mutex_);
		model_ = Ref<LLMModel>(memnew(LLMModel(model)));
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
	return model_.value_or(Ref<LLMModel>());
}

String LLMEngine::last_error() {
	std::shared_lock lock(mutex_);
	return String(last_error_.c_str());
}
