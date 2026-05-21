#include "game_db.hpp"

#include <filesystem>
#include <godot_cpp/classes/image.hpp>
#include <ranges>

#include "utils.hpp"

namespace fs = std::filesystem;

void GameDB::_bind_methods() {
	ClassDB::bind_method(D_METHOD("size"), &GameDB::size);
	ClassDB::bind_method(D_METHOD("clear"), &GameDB::clear);
	ClassDB::bind_method(D_METHOD("get_tree", "prefix"), &GameDB::get_tree, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_image_pack", "path"), &GameDB::get_image_pack);
}

size_t GameDB::size() const {
	 return try_invoke([&] {
		return 1;
	});
}

void GameDB::clear() {
	try_invoke([&] {
		return;
	});
}

String GameDB::get_tree(const String &prefix) {
	return try_invoke([&] {
		std::vector<std::string> item_names { "a", "b", "c" };
		auto tree = item_names | std::views::join_with('\n') | std::ranges::to<std::string>();
		return to_godot_string(tree);
	});
}

Ref<ImagePack> GameDB::get_image_pack(const String &path) {
	return try_invoke([&] {
		Ref<ImagePack> ret = memnew(ImagePack);
		return ret;
	});
}
