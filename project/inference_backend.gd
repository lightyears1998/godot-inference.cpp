extends Node


signal llm_loaded(model: LLM)
signal asr_model_loaded(model: ASRModel)

var llm: LLM
var asr_model: ASRModel


func _ready() -> void:
	_load_llm()
	_load_asr()


func _load_llm():
	var model_path: String = ProjectSettings.get_setting("llama.cpp/model_path")
	llm = InferenceEngine.request_load_llm(model_path)
	await llm.model_loaded
	llm_loaded.emit(llm)
	print("llm_loaded")


func _load_asr():
	var asr_model_path: String = ProjectSettings.get_setting("llama.cpp/asr_model_path")
	asr_model = InferenceEngine.request_load_asr_model(asr_model_path)
	await asr_model.model_loaded
	asr_model_loaded.emit(asr_model)
	print("asr_model_loaded")


func _physics_process(_delta: float) -> void:
	InferenceEngine.tick()
