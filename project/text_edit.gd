extends TextEdit


signal text_commited(text: String)


func _gui_input(event: InputEvent) -> void:
	if not editable:
		return

	if event is InputEventKey:
		if not event.is_pressed():
			return

		if event.keycode == KEY_ENTER || event.keycode == KEY_KP_ENTER:
			if event.shift_pressed:
				insert_text_at_caret("\n")
			else:
				commit_text()
				get_viewport().set_input_as_handled()


func commit_text():
	text_commited.emit(text)
	text = ""


func scroll_to_end_of_text():
	set_caret_line(get_line_count() - 1)
	set_caret_column(get_line(get_line_count() - 1).length() - 1)
