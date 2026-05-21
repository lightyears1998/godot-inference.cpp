#include "image_pack.hpp"
#include "utils.hpp"

void ImagePack::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &ImagePack::get_size);
	ClassDB::bind_method(D_METHOD("get_offset", "index"), &ImagePack::get_offset);
	ClassDB::bind_method(D_METHOD("get_image", "index"), &ImagePack::get_image);
}

int ImagePack::get_size() const {
	return try_invoke([&] {
		return 1;
	});
}

Vector2 ImagePack::get_offset(int index) const {
	return try_invoke([&] {
		return Vector2(222, 333);
	});
}

Ref<Image> ImagePack::get_image(int index) const {
	return try_invoke([&] {
		Ref<Image> ret = memnew(Image);
		ret->load_from_file("res://icon.svg");
		return ret;
	});
}
