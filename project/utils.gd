extends RefCounted
class_name Utils


var _print_stub: bool


func _init(debug_enabled: bool) -> void:
	_print_stub = debug_enabled


func benchmark(f: Callable, tag: String) -> void:
	var t_start = Time.get_ticks_msec()
	f.call()
	var t_end = Time.get_ticks_msec()
	print(tag, ": ", t_end - t_start, "ms")


func stub(...args: Array) -> void:
	if _print_stub:
		print.callv(args)
