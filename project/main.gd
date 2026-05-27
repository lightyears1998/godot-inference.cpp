extends Node

@onready var user_text_edit: TextEdit = %UserTextEdit
@onready var agent_text_edit: TextEdit = %AgentTextEdit
@onready var user_confirm_button: Button = %UserConfirmButton


func _ready() -> void:
	user_text_edit.text_commited.connect(submit_text)
	user_confirm_button.pressed.connect(func (): user_text_edit.commit_text())
	_test()


func submit_text(text: String) -> void:
	agent_text_edit.text += text + "\n"


func _general_chat() -> void:
	var model := LLMEngine.get_model()

	var t1 = Time.get_ticks_msec()
	var chat := model.start_general_chat()
	var t2 = Time.get_ticks_msec()
	print(t2 - t1, ' ms to create chat')

	chat.say("你好！")
	chat.request_reply()
	await chat.reply_generated
	print(chat.get_last_reply())


func _test() -> void:
	await LLMBackend.model_loaded
	await _general_chat()
