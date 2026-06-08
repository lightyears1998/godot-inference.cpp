#include "register_types.hpp"

#include "benchmark.hpp"
#include "constants.hpp"
#include "globals.hpp"
#include "llm_chat.hpp"
#include "llm_chat_message.hpp"
#include "llm_chat_parameters.hpp"
#include "llm_engine.hpp"
#include "llm_model.hpp"

#include <clocale>
#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
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

	ClassDB::register_abstract_class<LLMEngine>();
	ClassDB::register_class<LLMModel>();
	ClassDB::register_class<LLMChatParameters>();
	ClassDB::register_class<LLMChat>();
	ClassDB::register_class<LLMChatMessage>();
	ClassDB::register_class<Benchmark>();

	auto *ps = ProjectSettings::get_singleton();
	if (!ps->has_setting(SETTINGS_KEY_MODEL_PATH)) {
		ps->set_setting(SETTINGS_KEY_MODEL_PATH, "model.gguf");
	}

	if (!Engine::get_singleton()->is_editor_hint()) {
		LLMEngine::init_backend();
	}
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (!Engine::get_singleton()->is_editor_hint()) {
		LLMEngine::free_backend();
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
