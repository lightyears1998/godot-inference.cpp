#include "asr_model.hpp"

#include "utils.hpp"

#include <miniaudio/miniaudio.h>
#include <gsl/gsl>

using namespace godot;

ASRModel::ASRModel(const godot::String &path) {
	job_thread_ = std::jthread([=, this](std::stop_token stoken) {
		job_routine(std::move(stoken), path);
	});
}

void ASRModel::request_transcription(godot::Ref<godot::AudioStreamWAV> audio) {
	std::lock_guard lock(mutex_);

	audio_queue_.emplace_back(std::move(audio));
	cv_.notify_all();
}

void ASRModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("request_transcription", "audio"), &ASRModel::request_transcription);

	ADD_SIGNAL(MethodInfo("model_loaded"));
	ADD_SIGNAL(MethodInfo("speech_transcribed", PropertyInfo(Variant::STRING, "text")));
}

void ASRModel::job_routine(std::stop_token stoken, godot::String path) {
	try_invoke([&]() {
		init_context(stoken, path);
	});

	while (!stoken.stop_requested()) {
		{
			std::unique_lock lock(mutex_);
			cv_.wait(lock, stoken, [&]() {
				return stoken.stop_requested() || !audio_queue_.empty();
			});
		}

		std::deque<Ref<AudioStreamWAV>> audio_to_process;
		{
			std::unique_lock lock(mutex_);
			while (!audio_queue_.empty()) {
				audio_to_process.push_back(audio_queue_.front());
				audio_queue_.pop_front();
			}
		}

		while (!audio_to_process.empty()) {
			Ref<AudioStreamWAV> audio = audio_to_process.front();
			audio_to_process.pop_front();

			auto params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
			params.language = "auto";

			if (!audio->is_stereo() || audio->get_format() != AudioStreamWAV::Format::FORMAT_16_BITS) {
				print_error("Fail to transcribe: invalid audio channel or format: ", audio->is_stereo(), " ", audio->get_format());
				continue;
			}

			auto pcm16s = audio->get_data(); // PCM signed 16bit Stereo

			ma_data_converter_config cfg = ma_data_converter_config_init(ma_format_s16, ma_format_f32, 2, 1, audio->get_mix_rate(), 16000);
			ma_data_converter converter;
			if (auto ret = ma_data_converter_init(&cfg, nullptr, &converter); ret != MA_SUCCESS) {
				print_error(vformat("ma_data_converter_init failed: ret=%d", ret));
			}
			auto _free_converter = gsl::finally([&]() {
				ma_data_converter_uninit(&converter, nullptr);
			});

			std::vector<float> pcmf32;
			ma_uint64 n_input_frames = pcm16s.size() / sizeof(int16_t) / 2;
			ma_uint64 n_output_frames = static_cast<double>(n_input_frames) / audio->get_mix_rate() * 16000;
			pcmf32.resize(n_output_frames);

			if (auto ret = ma_data_converter_process_pcm_frames(&converter, pcm16s.ptr(), &n_input_frames, pcmf32.data(), &n_output_frames); ret != MA_SUCCESS) {
				print_error(vformat("ma_data_converter_process_pcm_frames failed: ret=%d", ret));
			}

			auto ctx = ctx_.get();
			if (whisper_full_parallel(ctx, params, pcmf32.data(), pcmf32.size(), 1) != 0) {
				print_error("whisper_full_parallel failed!");
				return;
			}

			std::string text;
			int n_segments = whisper_full_n_segments(ctx);
			for (int i = 0; i < n_segments; i++) {
				const char* seg_text = whisper_full_get_segment_text(ctx, i);
				text += seg_text;
			}

			call_deferred("emit_signal", "speech_transcribed", gstr(text));
		}
	}
}

void ASRModel::init_context(const std::stop_token& stoken, const godot::String& path) {
	auto ctx_params = whisper_context_default_params();
	auto ctx = whisper_init_from_file_with_params(path.utf8().ptr(), ctx_params);
	if (ctx == nullptr) {
		print_error("Fail to load model: ", path);
	}
	ctx_ = whisper_context_ptr(ctx, &whisper_free);

	call_deferred("emit_signal", "model_loaded");
}
