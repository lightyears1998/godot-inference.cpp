extends Node


func _ready() -> void:
	var db = GameDB.new()
	print('db.size() = ', db.size())
	print('db.get_tree("foo") = ', JSON.stringify(db.get_tree("foo")))

	var pck = db.get_image_pack("bar")
	print('pck.get_reference_count() = ', pck.get_reference_count())
	print('pck.get_offset(0) = ', pck.get_offset(0))
