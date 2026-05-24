#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/image.hpp>
#include <shared_mutex>

#include "llm_model.hpp"

class LLMEngine : public godot::Object {
	GDCLASS(LLMEngine, Object)

protected:
	static void _bind_methods();

public:
	enum ModelStatus {
		MODEL_EMPTY = 0,
		MODEL_LOADING = 1,
		MODEL_READY = 2
	};

	LLMEngine() = default;
	~LLMEngine() override = default;

	static void init_backend();
	static void tick();
	static void free_backend();
	static void request_load_model(const godot::String& path);
	static ModelStatus get_model_load_status();
	static float get_model_load_progress();
	static godot::Ref<LLMModel> get_model();

private:
	inline static std::optional<godot::Ref<LLMModel>> model_;
	inline static std::optional<std::jthread> backend_thread_;

	void load_model(std::string path);
};

VARIANT_ENUM_CAST(LLMEngine::ModelStatus);
