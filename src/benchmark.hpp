#pragma once

#include "godot_cpp/classes/ref_counted.hpp"

class Benchmark : public godot::RefCounted {
	GDCLASS(Benchmark, RefCounted)

public:
	void test_obj_id_lookup();

protected:
	static void _bind_methods();
};
