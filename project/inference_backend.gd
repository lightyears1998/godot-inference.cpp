extends Node


signal model_loaded(model: LLMModel)


var _is_ready := false


func _ready() -> void:
	InferenceEngine.request_load_model()
	await _report_load_progress()


func _physics_process(_delta: float) -> void:
	InferenceEngine.tick()


func _report_load_progress() -> void:
	var t_start := Time.get_ticks_msec()

	while is_inside_tree():
		var status := InferenceEngine.get_model_load_status()
		var progress := InferenceEngine.get_model_load_progress()
		print('model load progress: ', progress)
		if status != InferenceEngine.MODEL_LOADING:
			break
		await get_tree().create_timer(1).timeout

	if InferenceEngine.get_model_load_status() == InferenceEngine.MODEL_READY:
		print('model loaded')

		var t_end := Time.get_ticks_msec()
		print('time to load model: ', t_end - t_start, 'ms')

		_is_ready = true
		model_loaded.emit(InferenceEngine.get_model())


func wait_until_ready() -> void:
	if not _is_ready:
		await model_loaded
	return
