#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/image.hpp>
#include <shared_mutex>

#include "image_pack.hpp"

using namespace godot;

class GameDB : public RefCounted {
	GDCLASS(GameDB, RefCounted)

protected:
	static void _bind_methods();

public:
	GameDB() = default;
	~GameDB() override = default;

	size_t size() const;
	void clear();
	String get_tree(const String &prefix);
	Ref<ImagePack> get_image_pack(const String &path);
};
