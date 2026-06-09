extends Control

@onready var user_text_edit: TextEdit = %UserTextEdit
@onready var user_confirm_button: Button = %UserConfirmButton
@onready var clear_history_button: Button = $ClearHistoryButton
@onready var scroll_container: ScrollContainer = $ScrollContainer
@onready var vbox_container: VBoxContainer = $ScrollContainer/VBoxContainer

const VoiceLineScene := preload("res://voice_line.tscn")

var chat: LLMChat

var sys_prompt := """You are a helpeful assistant."""


func _ready() -> void:
	user_text_edit.text_commited.connect(submit_text)
	user_confirm_button.pressed.connect(func (): user_text_edit.commit_text())
	clear_history_button.pressed.connect(clear_chat_history)
	_setup_chat()


func submit_text(text: String) -> void:
	if not is_instance_valid(chat) or not chat.is_valid():
		return

	_add_voice_line("User", text, "", false)
	chat.say(text)
	chat.request_reply()


func clear_chat_history() -> void:
	if not is_instance_valid(chat):
		return
	chat.clear_history()

	for child in vbox_container.get_children():
		child.queue_free()


func _setup_chat() -> void:
	await InferenceBackend.model_loaded
	var model := InferenceEngine.get_model()

	chat = model.start_general_chat()
	print('chat created')
	chat.add_message("system", sys_prompt)
	chat.reply_generated.connect(_on_reply_generated)


func _add_voice_line(speaker: String, text: String, voice_line: String, voice_acting_enabled: bool) -> void:
	var scene := VoiceLineScene.instantiate()
	scene.debug_enabled = true
	scene.line = text
	scene.voice_line = voice_line
	scene.voice_acting_enabled = voice_acting_enabled
	scene.get_node("Speaker").text = speaker
	scene.get_node("LineLabel").text = text
	vbox_container.add_child(scene)
	await get_tree().process_frame
	scroll_container.scroll_vertical = int(scroll_container.get_v_scroll_bar().max_value)


func _on_reply_generated(content: String):
	var packs := content.split('\n')
	print('_on_reply_generated ', packs)

	if len(packs) < 2:
		push_error('wrong size! ', packs)
		return

	var text := packs[0]
	var voice_line := packs[1]
	_add_voice_line("assistant", text, voice_line, true)
