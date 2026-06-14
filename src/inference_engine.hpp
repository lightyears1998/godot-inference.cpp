#pragma once

#include "llm_model.hpp"

#include <atomic>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <thread>
#include <optional>
#include <shared_mutex>
#include <string>

class ASRModel;

// TODO Add support for remote api endpoint
// TODO Add ModelBase
class InferenceEngine final : public godot::Object {
	GDCLASS(InferenceEngine, godot::Object)

public:
	void init_backend();
	void tick();
	void free_backend();

	godot::Ref<LLM> request_load_llm(const godot::String& path);
	godot::Ref<ASRModel> request_load_asr_model(const godot::String& path);
	void use_remote_llm_endpoint(); // TODO

protected:
	static void _bind_methods();

private:
	std::mutex mutex_;
	std::set<godot::ObjectID> model_ids_; // TODO
};
