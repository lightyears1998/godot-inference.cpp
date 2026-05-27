#include "register_types.hpp"

#include "constants.hpp"
#include "globals.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "llm_chat.hpp"
#include "llm_chat_message.hpp"
#include "llm_chat_parameters.hpp"
#include "llm_engine.hpp"
#include "llm_model.hpp"

#include <gdextension_interface.h>
#include <clocale>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	main_thread_id = std::this_thread::get_id();

	ClassDB::register_abstract_class<LLMEngine>();
	ClassDB::register_class<LLMModel>();
	ClassDB::register_class<LLMChatParameters>();
	ClassDB::register_class<LLMChat>();
	ClassDB::register_class<LLMChatMessage>();

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

	/*
	 * Workaround for GGML's terminate handler assertion when hot-reloading is enabled.
	 *
	 * GGML installs its own terminate handler (ggml_uncaught_exception_init) to print backtraces.
	 * It contains an assertion that checks whether the current terminate handler is itself.
	 * During hot-reload, the assertion could be triggered, or worse, it might not be able to protect against circular reference.
	 * We reset the terminate handler here to avoid triggering the assertion in future reload.
	 */
	std::set_terminate(library_terminate_handler);

	if (!Engine::get_singleton()->is_editor_hint()) {
		LLMEngine::free_backend();
	}
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
