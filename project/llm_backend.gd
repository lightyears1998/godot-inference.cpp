extends Node


func _init() -> void:
	LLMEngine.init_backend()


func _ready() -> void:
	LLMEngine.request_load_model()
	await _report_load_progress()


func _process(_delta: float) -> void:
	LLMEngine.tick()


func _exit_tree() -> void:
	LLMEngine.free_backend()


func _report_load_progress() -> void:
	while true:
		var status := LLMEngine.get_model_load_status()
		var progress := LLMEngine.get_model_load_progress()
		print('model load progress: ', progress)
		if status != LLMEngine.MODEL_LOADING:
			break
		await get_tree().create_timer(1).timeout
