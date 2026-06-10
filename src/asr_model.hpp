#pragma once

#include <whisper.h>
#include <condition_variable>
#include <deque>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

class ASRModel : public godot::RefCounted {
	GDCLASS(ASRModel, RefCounted)

public:
	using whisper_context_ptr = std::unique_ptr<whisper_context, decltype(&whisper_free)>;

	ASRModel() = default;
	ASRModel(const godot::String& path);

	void request_transcription(godot::Ref<godot::AudioStreamWAV> audio);

protected:
	static void _bind_methods();

private:
	mutable std::mutex mutex_;
	std::condition_variable_any cv_;
	std::jthread job_thread_;

	std::deque<godot::Ref<godot::AudioStreamWAV>> audio_queue_;
	whisper_context_ptr ctx_ { nullptr, &whisper_free };

	void job_routine(std::stop_token stoken, godot::String path);
	void init_context(const std::stop_token& stoken, const godot::String& path);
};
