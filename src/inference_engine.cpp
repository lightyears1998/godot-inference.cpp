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

using namespace godot;

void InferenceEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("tick"), &InferenceEngine::tick);
	ClassDB::bind_method(D_METHOD("request_load_llm", "path"), &InferenceEngine::request_load_llm);
	ClassDB::bind_method(D_METHOD("request_load_asr_model", "path"), &InferenceEngine::request_load_asr_model);
}

void InferenceEngine::init_backend() {
	print_line(BOOST_CURRENT_FUNCTION);

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
	std::lock_guard lock(mutex_);

	for (auto it = model_ids_.begin(); it != model_ids_.end(); ) {
		auto model = static_cast<LLM*>(ObjectDB::get_instance(*it)); // TODO
		if (model == nullptr) {
			it = model_ids_.erase(it);
		} else {
			model->tick();
			++it;
		}
	}
}
void InferenceEngine::free_backend() {
	print_line(BOOST_CURRENT_FUNCTION);

	llama_backend_free();
}


Ref<LLM> InferenceEngine::request_load_llm(const String& path) {
	if (path.is_empty()) {
		print_error("Model path cannot be empty.");
		return {};
	}

	Ref<LLM> model = memnew(LLM);
	model->load_model(path);

	{
		std::lock_guard lock(mutex_);
		model_ids_.insert(godot::ObjectID(model->get_instance_id()));
	}

	return model;
}

Ref<ASRModel> InferenceEngine::request_load_asr_model(const godot::String &path) {
	Ref<ASRModel> model = memnew(ASRModel(path));
	return model;
}
