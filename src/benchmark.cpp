#include "benchmark.hpp"

#include "llm_chat.hpp"

#include <functional>

using namespace godot;

struct Result {
	std::chrono::time_point<std::chrono::steady_clock> t_start;
	std::chrono::time_point<std::chrono::steady_clock> t_end;

	double duration_ms() const {
		return std::chrono::duration<double, std::milli>(t_end - t_start).count();
	}
};

Result benchmark(const std::function<void()> &f, int times = 1) {
	Result ret;

	ret.t_start = std::chrono::steady_clock::now();
	while (times--)
		f();
	ret.t_end = std::chrono::steady_clock::now();

	return ret;
}

/**
 * benchmark: 1000 object_id lookup (static_cast): 0.0339ms
 * benchmark: 1000 object_id lookup (dynamic_cast): 0.0782ms
 * benchmark: 1000 object_id lookup (Object::cast_to): 0.1622ms
 */
void Benchmark::test_obj_id_lookup() {
	std::vector<godot::Ref<LLMChat>> chats;
	std::vector<ObjectID> object_ids;

	for (int i = 0; i < 1000; ++i) {
		Ref<LLMChat> chat = memnew(LLMChat());
		chats.emplace_back(chat);
		object_ids.emplace_back(chat->get_instance_id());
	}

	auto job1 = [&]() {
		int ok = 0;

		for (auto id : object_ids) {
			auto obj = ObjectDB::get_instance(id);
			auto casted_chat = static_cast<LLMChat*>(obj);
			if (casted_chat != nullptr) {
				++ok;
			}
		}

		if (ok != 1000) {
			print_error("bad thing happened."); // never
		}
	};

	auto job2 = [&]() {
		int ok = 0;

		for (auto id : object_ids) {
			auto obj = ObjectDB::get_instance(id);
			auto casted_chat = cast_to<LLMChat>(obj);
			if (casted_chat != nullptr) {
				++ok;
			}
		}

		if (ok != 1000) {
			print_error("bad thing happened."); // never
		}
	};

	auto ret1 = benchmark(job1);
	auto ret2 = benchmark(job2);
	chats.clear();

	print_line("benchmark: 1000 object_id lookup (dangerous): ", ret1.duration_ms(), "ms");
	print_line("benchmark: 1000 object_id lookup (Object::cast_to): ", ret2.duration_ms(), "ms");
}

void Benchmark::_bind_methods() {
	ClassDB::bind_method(D_METHOD("test_obj_id_lookup"), &Benchmark::test_obj_id_lookup);
}
