#include "utils.hpp"

#include <simdutf.h>

using namespace godot;

godot::String to_godot_string(const std::string &str) {
	godot::String ret;
	ret.parse_utf8(str.c_str(), str.length());
	return ret;
}

/*
 * Note: Godot string is UTF32.
 * https://docs.godotengine.org/en/stable/engine_details/architecture/core_types.html#containers
 */
std::string to_std_string(const godot::String &str) {
	const auto u32_ptr = str.ptr();
	const auto u32_len = str.length();
	const auto u8_len = simdutf::utf8_length_from_utf32(u32_ptr, u32_len);

	std::string ret(u8_len, '\0');
	(void)simdutf::convert_valid_utf32_to_utf8(u32_ptr, u32_len, ret.data());
	return ret;
}
