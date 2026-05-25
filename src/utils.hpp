#pragma once

#include <functional>
#include <string>
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/string.hpp>

godot::String to_godot_string(const std::string &str);
std::string to_std_string(const godot::String &str);

auto gstr(const auto& what) { return to_godot_string(what); }
auto ustr(const auto& what) { return to_std_string(what); }

template <typename F, typename... Args>
[[nodiscard]] auto try_invoke(F &&func, Args&&... args) {
	using T = decltype(std::invoke(std::forward<F>(func), std::forward<Args>(args)...));

	try {
		if constexpr (std::is_void_v<T>) {
			std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
		} else {
			return std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
		}
	} catch (std::exception &e) {
		godot::print_error(e.what());
	} catch (...) {
		godot::print_error("Unknown exception");
	}

	if constexpr (!std::is_void_v<T>) {
		return T {};
	}
}
