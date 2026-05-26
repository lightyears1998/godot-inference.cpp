#include "globals.hpp"

#include "godot_cpp/core/print_string.hpp"

using namespace godot;

// Inspired by cpptrace::register_terminate_handler.
// <https://github.com/jeremy-rifkin/cpptrace/blob/91b6b78e408a8b1c0b7146c9034a03156c082da2/src/utils.cpp#L58>
void library_terminate_handler() {
	try {
		auto ptr = std::current_exception();
		if (ptr == nullptr) {
			print_line("terminate called without an active exception.");
		} else {
			std::rethrow_exception(ptr);
		}
	} catch (const std::exception &e) {
		print_error("terminate called after throwing an unknown exception: ", typeid(e).name(), e.what());
	} catch (...) {
		print_error("terminate called after throwing an unknown exception.");
	}

	std::abort();
}
