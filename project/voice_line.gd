extends HBoxContainer


enum AudioStatus {
	NONE,
	BUFFERING,
	STREAMING,
	STREAMING_FINISHED
}

signal audio_is_buffered

@export var debug_enabled: bool
@export var line: String
@export var voice_line: String
@export var voice_acting_enabled: bool = true

@onready var audio_stream_player: AudioStreamPlayer = $AudioStreamPlayer
@onready var generator: AudioStreamGenerator = audio_stream_player.stream
@onready var playback: AudioStreamGeneratorPlayback = null
@onready var play_audio_button: Button = $PlayAudioButton
@onready var line_label: Label = $LineLabel


var _utils: Utils
var _audio_status := AudioStatus.NONE
var _pcm16le: PackedByteArray
var _should_play: bool


func _ready() -> void:
	_utils = Utils.new(debug_enabled)
	await InferenceBackend.wait_until_ready()

	play_audio_button.pressed.connect(_toggle_play)
	line_label.text = line

	if voice_acting_enabled:
		_utils.stub("start to request voice: ", line)
		if voice_line.is_empty():
			voice_line = line
		request_voice(voice_line)

		await audio_is_buffered
		_utils.stub('start to play voice: ', voice_line)
		_play()
	else:
		play_audio_button.disabled = true


func load_pcm_from_file(filepath: String) -> void:
	_pcm16le = FileAccess.get_file_as_bytes(filepath)
	_audio_status = AudioStatus.STREAMING_FINISHED


func request_voice(text: String) -> void:
	_audio_status = AudioStatus.BUFFERING
	var task_id := randi()

	var http_client := HTTPClient.new()
	var err = http_client.connect_to_host("127.0.0.1", 8000)
	if err != OK:
		push_error(task_id, 'connect_to_host: ', err)
		return

	while http_client.get_status() == HTTPClient.STATUS_CONNECTING || http_client.get_status() == HTTPClient.STATUS_RESOLVING:
		http_client.poll()
		await get_tree().process_frame

	if http_client.get_status() != HTTPClient.STATUS_CONNECTED:
		push_error(task_id, 'get_status ', http_client.get_status())
		return
	_utils.stub(task_id, 'connected')

	var request_body = {
		"model": "tts-1",
		"input": text,
		"response_format": "pcm"
	}
	var json_string = JSON.stringify(request_body)
	var headers = [
		"Content-Type: application/json"
	]

	err = http_client.request(
		HTTPClient.METHOD_POST,
		"/v1/audio/speech",
		headers,
		json_string
	)
	if err != OK:
		print("request: ", err)
		return

	while http_client.get_status() == HTTPClient.STATUS_REQUESTING:
		http_client.poll()
		await get_tree().process_frame

	var response_code = http_client.get_response_code()
	if response_code < 200 or response_code >= 300:
		push_error(task_id, 'response_code', response_code)
		return
	_utils.stub(task_id, "responding")

	var bytes: PackedByteArray
	while http_client.get_status() == HTTPClient.STATUS_BODY:
		bytes = http_client.read_response_body_chunk()
		_push_pcm_buffer(bytes, false)
		await get_tree().process_frame

	_push_pcm_buffer([], true)
	http_client.close()
	_utils.stub(task_id, 'request completed')


func _push_pcm_buffer(bytes: PackedByteArray, is_finished: bool) -> void:
	_pcm16le.append_array(bytes)

	if is_finished:
		var should_emit_buffered = _audio_status == AudioStatus.BUFFERING
		_audio_status = AudioStatus.STREAMING_FINISHED
		if should_emit_buffered:
			audio_is_buffered.emit()
	elif _audio_status == AudioStatus.BUFFERING:
		if _pcm16le.size() >= generator.mix_rate * (generator.buffer_length * 8) * (16/8):
			_audio_status = AudioStatus.STREAMING
			_utils.stub('buffered')
			audio_is_buffered.emit()


func _toggle_play() -> void:
	if audio_stream_player.playing:
		_stop()
	else:
		_play()


func _play() -> void:
	if _audio_status == AudioStatus.NONE:
		push_error('bad audio status: ', _audio_status)
		return

	_should_play = true
	audio_stream_player.play()
	playback = audio_stream_player.get_stream_playback()
	_play_pcm()


func _stop() -> void:
	_should_play = false


func _play_pcm() -> void:
	if _audio_status == AudioStatus.BUFFERING:
		await audio_is_buffered

	_utils.stub('playing')
	var i := 0
	while _should_play and (_audio_status == AudioStatus.STREAMING_FINISHED && i + 1 < _pcm16le.size() || _audio_status == AudioStatus.STREAMING):
		var frames := playback.get_frames_available()
		while i + 1 < _pcm16le.size() and frames > 0:
			var raw := _pcm16le.decode_s16(i)
			i += 2
			var val := clampf(float(raw) / 32768, -1, 1)
			playback.push_frame(Vector2.ONE * val)
			frames -= 1
		await get_tree().process_frame
	_utils.stub('end of filling')

	var last_skips := playback.get_skips()
	while _should_play and playback.get_skips() == last_skips:
		await get_tree().process_frame
	playback.stop()
	playback.clear_buffer()
	_utils.stub("playback stopped")
