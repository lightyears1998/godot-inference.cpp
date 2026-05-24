#include "llm_engine.hpp"

#include <llama.h>
#include <filesystem>
#include <godot_cpp/classes/image.hpp>
#include <ranges>

#include "utils.hpp"

using namespace godot;

namespace fs = std::filesystem;


void LLMEngine::_bind_methods() {
	BIND_ENUM_CONSTANT(MODEL_EMPTY);
	BIND_ENUM_CONSTANT(MODEL_LOADING);
	BIND_ENUM_CONSTANT(MODEL_READY);

	// ClassDB::bind_static_method(D_METHOD("init_backend"), &LLMEngine::init_backend);
}

void LLMEngine::init_backend() {
}

void LLMEngine::free_backend() {
}

void LLMEngine::request_load_model(const String &path) {
}

LLMEngine::ModelStatus LLMEngine::get_model_load_status() {
	throw std::exception("Not implemented"); // TODO
}
