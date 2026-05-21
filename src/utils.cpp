#include "utils.hpp"

#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace godot;

godot::String to_godot_string(const std::string &str) {
	godot::String ret;
	ret.parse_utf8(str.c_str());
	return ret;
}

std::string to_std_string(const godot::String &str) {
	const auto bytes = str.to_utf8_buffer();
	return { reinterpret_cast<const char*>(bytes.ptr()), static_cast<size_t>(bytes.size()) };
}
