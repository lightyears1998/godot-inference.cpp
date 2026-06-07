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
class LLMEngine final : public godot::RefCounted {
	GDCLASS(LLMEngine, godot::RefCounted)

public:
	enum ModelStatus {
		MODEL_EMPTY = 0,
		MODEL_LOADING = 1,
		MODEL_READY = 2,
	};

	static void init_backend();
	static void tick();
	static void free_backend();
	static void request_load_model(godot::String path);
	static void unload_model();
	static ModelStatus model_load_status();
	static float model_load_progress();
	static godot::Ref<LLMModel> model();
	static godot::String last_error();

protected:
	static void _bind_methods();

private:
	inline static std::shared_mutex mutex_;
	inline static godot::Ref<LLMModel> model_;
	inline static std::jthread background_thread_;
	inline static std::atomic<ModelStatus> model_status_ = MODEL_EMPTY;
	inline static std::atomic<float> load_progress_ = 0;
	inline static std::string last_error_;

	static void load_model(std::stop_token, std::string path);
};

VARIANT_ENUM_CAST(LLMEngine::ModelStatus);
