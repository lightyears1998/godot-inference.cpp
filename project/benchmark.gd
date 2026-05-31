extends VBoxContainer


@onready var test_button: Button = $TestButton


func _ready() -> void:
	test_button.pressed.connect(_test)


func _test() -> void:
	var b = Benchmark.new();
	b.test_obj_id_lookup()
