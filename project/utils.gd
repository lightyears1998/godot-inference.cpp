extends RefCounted
class_name Utils


func benchmark(f: Callable, tag: String) -> void:
	var t_start = Time.get_ticks_msec()
	f.call()
	var t_end = Time.get_ticks_msec()
	print(tag, ": ", t_end - t_start, "ms")
