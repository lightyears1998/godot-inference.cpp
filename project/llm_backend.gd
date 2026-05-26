extends Node


signal model_loaded(model: LLMModel)


func _ready() -> void:
	LLMEngine.request_load_model()
	await _report_load_progress()


func _process(_delta: float) -> void:
	LLMEngine.tick()


func _report_load_progress() -> void:
	var t_start := Time.get_ticks_msec()

	while is_inside_tree():
		var status := LLMEngine.get_model_load_status()
		var progress := LLMEngine.get_model_load_progress()
		print('model load progress: ', progress)
		if status != LLMEngine.MODEL_LOADING:
			break
		await get_tree().create_timer(1).timeout

	if LLMEngine.get_model_load_status() == LLMEngine.MODEL_READY:
		print('model loaded')

		var t_end := Time.get_ticks_msec()
		print('time to load model: ', t_end - t_start, 'ms')

		model_loaded.emit(LLMEngine.get_model())
