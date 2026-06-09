extends Node

@onready var user_text_edit: TextEdit = %UserTextEdit
@onready var agent_text_edit: TextEdit = %AgentTextEdit
@onready var user_confirm_button: Button = %UserConfirmButton
@onready var clear_history_button: Button = $ClearHistoryButton


var chat: LLMChat


func _ready() -> void:
	user_text_edit.text_commited.connect(submit_text)
	user_confirm_button.pressed.connect(func (): user_text_edit.commit_text())
	clear_history_button.pressed.connect(clear_chat_history)
	_setup_chat()


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


func _setup_chat() -> void:
	await InferenceBackend.model_loaded
	var model := InferenceEngine.get_model()

	var t1 = Time.get_ticks_msec()
	chat = model.start_general_chat()
	var t2 = Time.get_ticks_msec()
	print(t2 - t1, ' ms to create chat')
	chat.add_message("system", "You are a helpful assistant.")
	chat.piece_generated.connect(_append_text)


func _append_text(t: String) -> void:
	agent_text_edit.text += t;
	agent_text_edit.scroll_to_end_of_text()
