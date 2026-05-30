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
	await LLMBackend.model_loaded
	var model := LLMEngine.get_model()

	var t1 = Time.get_ticks_msec()
	chat = model.start_general_chat()
	var t2 = Time.get_ticks_msec()
	print(t2 - t1, ' ms to create chat')
	chat.add_message("system", "你是用户的妹妹。当用户与你交谈时，无论他使用什么语言，都请先使用中文回答，然后使用日语回答。中文和日语回答中没有换行符，回答结束的末尾有且只有一个换行符。不要输出多余的换行符。")
	chat.piece_generated.connect(func (t): call_deferred("_append_text", t))


func _append_text(t: String) -> void:
	agent_text_edit.text += t;
	agent_text_edit.scroll_to_end_of_text()
