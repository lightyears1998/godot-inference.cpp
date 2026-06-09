#include "register_types.hpp"

#include "benchmark.hpp"
#include "constants.hpp"
#include "globals.hpp"
#include "inference_engine.hpp"
#include "llm_chat.hpp"
#include "llm_chat_message.hpp"
#include "llm_chat_parameters.hpp"
#include "llm_model.hpp"

#include <gdextension_interface.h>
#include <clocale>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <tracy/Tracy.hpp>

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef TRACY_ENABLE
	tracy::StartupProfiler();
#endif

	main_thread_id = std::this_thread::get_id();

	ClassDB::register_class<InferenceEngine>();
	ClassDB::register_class<LLMModel>();
	ClassDB::register_class<LLMChatParameters>();
	ClassDB::register_class<LLMChat>();
	ClassDB::register_class<LLMChatMessage>();
	ClassDB::register_class<Benchmark>();

	const auto godot_engine = Engine::get_singleton();

	if (engine_instance == nullptr) {
		engine_instance = memnew(InferenceEngine);
		godot_engine->register_singleton("InferenceEngine", engine_instance);

		if (!godot_engine->is_editor_hint()) {
			engine_instance->init_backend();
		}
	}
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	const auto godot_engine = Engine::get_singleton();

	if (engine_instance != nullptr) {
		godot_engine->unregister_singleton("InferenceEngine");
		if (!godot_engine->is_editor_hint()) {
			engine_instance->free_backend();
		}

		memdelete(engine_instance);
		engine_instance = nullptr;
	}

#ifdef TRACY_ENABLE
	tracy::ShutdownProfiler();
#endif
}

extern "C"
{
	GDExtensionBool GDE_EXPORT godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		setlocale(LC_ALL, ".UTF8");

		std::set_terminate(library_terminate_handler);

		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
