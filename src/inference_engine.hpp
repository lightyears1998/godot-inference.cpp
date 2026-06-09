#pragma once

#include "llm_model.hpp"

#include <atomic>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <thread>
#include <optional>
#include <shared_mutex>
#include <string>

// It is design to handle a single model right now.
// docs/TODO.md
class InferenceEngine final : public godot::Object {
	GDCLASS(InferenceEngine, godot::Object)

public:
	enum ModelStatus {
		MODEL_EMPTY = 0,
		MODEL_LOADING = 1,
		MODEL_READY = 2,
	};

	void init_backend();
	void tick();
	void free_backend();

	void request_load_model(const godot::String& path);
	void unload_model();
	ModelStatus model_load_status();
	float model_load_progress();
	godot::Ref<LLMModel> model();
	godot::String last_error();

protected:
	static void _bind_methods();

private:
	std::shared_mutex mutex_;
	godot::Ref<LLMModel> model_;
	std::jthread background_thread_;
	std::atomic<ModelStatus> model_status_ = MODEL_EMPTY;
	std::atomic<float> load_progress_ = 0;
	std::string last_error_;

	void load_model(std::stop_token, const std::string& path);
	void model_loaded(const godot::String &model_path, const godot::Ref<LLMModel>& model);
};

VARIANT_ENUM_CAST(InferenceEngine::ModelStatus);
