extends Node

@onready var user_text_edit: TextEdit = %UserTextEdit
@onready var agent_text_edit: TextEdit = %AgentTextEdit
@onready var user_confirm_button: Button = %UserConfirmButton
@onready var clear_history_button: Button = $ClearHistoryButton
@onready var toggle_mic_button: Button = %ToggleMicButton
@onready var mic_stream_player: AudioStreamPlayer = %MicStreamPlayer
@onready var mic_stream_recorder: AudioStreamPlayer = %MicStreamRecorder
@onready var model_load_progress_bar: ProgressBar = %ModelLoadProgressBar

@onready var mic_bus_idx := AudioServer.get_bus_index("MicRecord")
@onready var record_effect: AudioEffectRecord = AudioServer.get_bus_effect(mic_bus_idx, 0)

var chat: LLMChat
var asr_model: ASRModel


func _ready() -> void:
	_report_model_load_progress()
	user_text_edit.text_commited.connect(submit_text)
	user_confirm_button.pressed.connect(func (): user_text_edit.commit_text())
	clear_history_button.pressed.connect(clear_chat_history)
	toggle_mic_button.pressed.connect(_toggle_mic_recording)
	_setup_chat()
	InferenceBackend.asr_model_loaded.connect(_on_asr_model_loaded)


func _report_model_load_progress():
	while true:
		var progress := InferenceBackend.llm.get_load_progress()
		model_load_progress_bar.value = progress
		await get_tree().process_frame
		if progress >= 1.0:
			print('model loaded')
			return


func _on_asr_model_loaded(model: ASRModel) -> void:
	asr_model = model
	asr_model.speech_transcribed.connect(func (text): user_text_edit.text += text)


func submit_text(text: String) -> void:
	if not is_instance_valid(chat) or not chat.is_valid():
		return
	chat.say(text)
	agent_text_edit.text += text + "\n"
	chat.request_reply()


func clear_chat_history() -> void:
	if not is_instance_valid(chat):
		return
	chat.clear_history()
	agent_text_edit.text = ""


func _setup_chat() -> void:
	await InferenceBackend.llm_loaded
	var model := InferenceBackend.llm

	var t1 = Time.get_ticks_msec()
	chat = model.start_general_chat()
	var t2 = Time.get_ticks_msec()
	print(t2 - t1, ' ms to create chat')
	chat.add_message("system", "You are a helpful assistant.")
	chat.piece_generated.connect(_append_text)


func _append_text(t: String) -> void:
	agent_text_edit.text += t;
	agent_text_edit.scroll_to_end_of_text()


func _toggle_mic_recording() -> void:
	if record_effect.is_recording_active():
		var mic_record := record_effect.get_recording()
		record_effect.set_recording_active(false)
		mic_stream_recorder.stop()
		mic_stream_player.stream = mic_record
		mic_stream_player.play()
		toggle_mic_button.text = "MIC"
		asr_model.request_transcription(mic_record)
	else:
		mic_stream_recorder.play()
		record_effect.set_recording_active(true)
		toggle_mic_button.text = "..."
