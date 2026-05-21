#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/image.hpp>
#include <shared_mutex>

using namespace godot;

class ImagePack : public RefCounted {
	GDCLASS(ImagePack, RefCounted)

protected:
	static void _bind_methods();

public:
	ImagePack() = default;

	int get_size() const;
	Vector2 get_offset(int index) const;
	Ref<Image> get_image(int index) const;
};
