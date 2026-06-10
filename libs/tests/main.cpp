#include "core/utils.hpp"

#include <fmt/format.h>
#include <ggml.h>
#include <llama.h>
#include <miniaudio/miniaudio.h>
#include <whisper.h>
#include <chrono>
#include <clocale>
#include <filesystem>
#include <fstream>
#include <gsl/util>
#include <iostream>
#include <string>
#include <tracy/Tracy.hpp>
#include <vector>

int test1(std::string model_path) {
	// load dynamic backends
	llama_backend_init();

	// ggml_backend_load_all();

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = -1;

	llama_model *model = llama_model_load_from_file(model_path.c_str(), model_params);
	if (!model) {
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		return true;
	}

	size_t model_size = llama_model_size(model);

	const llama_vocab *vocab = llama_model_get_vocab(model);

	// initialize the sampler
	auto sampler_params = llama_sampler_chain_default_params();
	llama_sampler *smpl = llama_sampler_chain_init(sampler_params);
	// llama_sampler_chain_add(smpl, llama_sampler_init_penalties(64, 1., 0, 1.5));
	llama_sampler_chain_add(smpl, llama_sampler_init_top_k(20));
	llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.8, 0));
	llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0., 0));
	llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7));
	llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

	llama_sampler_seq_config config { .seq_id = 0, .sampler = smpl };

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = 16000;
	ctx_params.n_batch = 2048;
	ctx_params.n_ubatch = 512;
	ctx_params.type_k = ggml_type::GGML_TYPE_Q8_0;
	ctx_params.type_v = ggml_type::GGML_TYPE_Q8_0;
	ctx_params.samplers = &config; // NOTE: backedn sampler, might help or not
	ctx_params.n_samplers = 1;

	llama_context *ctx = llama_init_from_model(model, ctx_params);
	if (!ctx) {
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return true;
	}

	// std::string test = "<think></think>This is a bad apple, and that too.";
	// std::vector<llama_token> tokens(1024);
	// llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), true, true);
	// llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), true, false);
	// llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), false, false);
	//
	// for (auto token : tokens) {
	// 	auto attr = llama_token_get_attr(vocab, token);
	// 	std::puts(std::to_string(attr).c_str());
	// }

	// helper function to evaluate a prompt and generate a response
	auto generate = [&](const std::string &prompt) {
		ZoneScopedN("generate");

		std::string response;

		// tokenize the prompt
		const int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, false, true);
		std::vector<llama_token> prompt_tokens(n_prompt_tokens);
		if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), false, true) < 0) {
			GGML_ABORT("failed to tokenize the prompt\n");
		}

		constexpr auto clear_batch = [](llama_batch &batch) -> void {
			batch.n_tokens = 0;
		};

		constexpr auto add_token = [](llama_batch& batch, int pos, llama_token token, bool logits) -> void {
			batch.token[batch.n_tokens] = token;
			batch.pos[batch.n_tokens] = pos;
			batch.n_seq_id[batch.n_tokens] = 1;
			batch.seq_id[batch.n_tokens][0] = 0;
			batch.logits[batch.n_tokens] = logits;

			batch.n_tokens++;
		};

		constexpr auto fill_batch = [add_token](llama_batch &batch, const std::vector<llama_token> &tokens, const int start_pos) -> void {
			for (int i = 0; i < tokens.size(); ++i) {
				add_token(batch, start_pos + i, tokens[i], false);
			}
			batch.logits[batch.n_tokens - 1] = true;
		};

		// prepare a batch for the prompt
		llama_batch batch = llama_batch_init(prompt_tokens.size(), 0, 1);
		llama_token new_token_id;

		int seq_next_pos = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
		fill_batch(batch, prompt_tokens, seq_next_pos);
		seq_next_pos += prompt_tokens.size();

		llama_synchronize(ctx);
		int n_generated = 0;
		auto t_start = std::chrono::steady_clock::now();
		int n_ctx = llama_n_ctx(ctx);
		while (true) {
			// check if we have enough space in the context to evaluate this batch
			// int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
			// if (n_ctx_used + batch.n_tokens > n_ctx) {
			// 	printf("\033[0m\n");
			// 	fprintf(stderr, "context size exceeded\n");
			// 	exit(0);
			// }

			int ret;
			{
				ZoneScopedN("llama_decode");
				ret = llama_decode(ctx, batch);
			}
			if (ret != 0) {
				GGML_ABORT("failed to decode, ret = %d\n", ret);
			}

			// sample the next token
			{
				ZoneScopedN("llama_sampler_sample");
				new_token_id = llama_sampler_sample(smpl, ctx, -1);
			}
			++n_generated;

			if (n_generated == 1) {
				t_start = std::chrono::steady_clock::now();
			}

			// is it an end of generation?
			if (llama_vocab_is_eog(vocab, new_token_id)) {
				break;
			}

			// convert the token to a string, print it and add it to the response
			// char buf[256];
			// int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
			// if (n < 0) {
			// 	GGML_ABORT("failed to convert token to piece\n");
			// }
			// std::string piece(buf, n);
			// printf("%s", piece.c_str());
			// fflush(stdout);
			// response += piece;

			// prepare the next batch with the sampled token
			clear_batch(batch);
			add_token(batch, seq_next_pos++, new_token_id, true);
		}
		auto t_end = std::chrono::steady_clock::now();
		std::chrono::duration<double> duration = t_end - t_start;

		printf("\ntokens_per_sec: %f\n", n_generated / duration.count());

		return response;
	};

	std::vector<llama_chat_message> messages;
	std::vector<char> formatted(llama_n_ctx(ctx));
	int prev_len = 0;
	while (true) {
		// get user input
		printf("\033[32m> \033[0m");
		std::string user;
		std::getline(std::cin, user);

		if (user.empty()) {
			break;
		}

		const char *tmpl = llama_model_chat_template(model, /* name */ nullptr);

		// add the user input to the message list and format it
		messages.push_back({ "user", strdup(user.c_str()) });
		int new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
		if (new_len > (int)formatted.size()) {
			formatted.resize(new_len);
			new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
		}
		if (new_len < 0) {
			fprintf(stderr, "failed to apply the chat template\n");
			return true;
		}

		// remove previous messages to obtain the prompt to generate the response
		std::string prompt(formatted.begin() + prev_len, formatted.begin() + new_len);
		prompt += "<think>\n\n</think>\n\n";

		// generate a response
		printf("\033[33m");
		std::string response = generate(prompt);
		printf("\n\033[0m");

		// add the response to the messages
		messages.push_back({ "assistant", strdup(response.c_str()) });
		prev_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), false, nullptr, 0);
		if (prev_len < 0) {
			fprintf(stderr, "failed to apply the chat template\n");
			return true;
		}
	}

	// free resources
	for (auto &msg : messages) {
		free(const_cast<char *>(msg.content));
	}
	llama_sampler_free(smpl);
	llama_free(ctx);
	llama_model_free(model);

	return false;
}


void test_double_free(std::string model_path) {
	llama_backend_init();

	llama_backend_free();
	llama_backend_free();

	llama_backend_init();
	llama_backend_free();

	llama_backend_init();

	auto model = llama_model_load_from_file(model_path.c_str(), llama_model_default_params());

	auto ctx_params = llama_context_default_params();
	ctx_params.n_ctx = 2048;
	auto context = llama_init_from_model(model, ctx_params);

	llama_backend_free();
}

void test2(std::string model_path) {
	printf("Model: %s\n", model_path.c_str());
	printf("CWD: %s\n", std::filesystem::current_path().string().c_str());

	constexpr auto log_cb = [](ggml_log_level level, const char *text, void *_) {
		if (level < GGML_LOG_LEVEL_WARN) {
			return;
		}

		printf("%s\n", text);
	};

	whisper_log_set(log_cb, nullptr);

	auto ctx_params = whisper_context_default_params();
	auto ctx = whisper_init_from_file_with_params(model_path.c_str(), ctx_params);
	auto _ = gsl::finally([&]() {
		whisper_free(ctx);
	});

	auto params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
	params.language = "auto";
	params.detect_language = true;

	std::ifstream ifs("test.wav", std::ios::binary);

	ifs.seekg(0, std::ios::end);
	auto size = (size_t)ifs.tellg() - 44;
	ifs.seekg(44, std::ios::beg);

	std::vector<int16_t> pcm16(size / sizeof(int16_t));
	ifs.read(reinterpret_cast<char*>(pcm16.data()), size);

	std::vector<float> pcmf32;

	// ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 1, 16000);
	// ma_decoder decoder;
	// auto ret = ma_decoder_init_memory(pcm16.data(), pcm16.size() * sizeof(int16_t), &cfg, &decoder);
	// if (ret != MA_SUCCESS) {
	// 	printf("ma_decoder_init_memory failed: %d\n", ret);
	// 	return;
	// }
	// auto _ = gsl::finally([&]() {
	// 	ma_decoder_uninit(&decoder);
	// });

	ma_data_converter_config cfg = ma_data_converter_config_init(ma_format_s16, ma_format_f32, 2, 1, 44100, 16000);
	ma_data_converter cvt;
	auto ret = ma_data_converter_init(&cfg, nullptr, &cvt);
	if (ret != MA_SUCCESS) {
		printf("ma_data_converter_init: %d\n", ret);
	}
	auto _free_converter = gsl::finally([&]() {
		ma_data_converter_uninit(&cvt, nullptr);
	});

	ma_uint64 n_input_frames = pcm16.size() / 2;
	ma_uint64 n_output_frames = static_cast<double>(n_input_frames) / 44100 * 16000;
	pcmf32.resize(n_output_frames);

	ret = ma_data_converter_process_pcm_frames(&cvt, pcm16.data(), &n_input_frames, pcmf32.data(), &n_output_frames);
	if (ret != MA_SUCCESS) {
		printf("ma_data_converter_process_pcm_frames failed! %d\n", ret);
	}

	pcmf32.resize(n_output_frames);

	// std::ofstream ofs("test_out.pcm", std::ios::binary);
	// ofs.write(reinterpret_cast<const char *>(pcmf32.data()), pcmf32.size() * sizeof(float));

	if (params.detect_language) {
		if (whisper_full_parallel(ctx, params, pcmf32.data(), pcmf32.size(), 1) != 0) {
			printf("failed!\n");
			return;
		}
		params.detect_language = false;

		auto lang_id = whisper_full_lang_id(ctx);
		const char* lang_str = whisper_lang_str(lang_id);
		printf("auto-detected lang: %s\n", lang_str);
	}

	if (whisper_full_parallel(ctx, params, pcmf32.data(), pcmf32.size(), 1) != 0) {
		printf("failed!\n");
		return;
	}

	int n_segments = whisper_full_n_segments(ctx);
	for (int i = 0; i < n_segments; i++) {
		const char* text = whisper_full_get_segment_text(ctx, i);
		printf("[%s]", text);
	}
	printf("\n");
}


// .\llama-server.exe -m .\models\Qwen3.5-4B-Q4_K_M.gguf --reasoning off -ctk q8_0 -ctv q8_0
// --ctx-size 16384
// --ctx-size 32768 40t/s Best performance
// --ctx-size 65536 [28..10]t/s Performance will gradually degrade as VRAM runs out.
// --ctx-size 131072(2^17) 262144(2^18, MAX)
int main(int argc, char** argv) {
	std::setlocale(LC_ALL, ".UTF8");

	if (argc < 2) {
		return 1;
	}
    // path to the model gguf file
    std::string model_path = core::acp_to_utf8(argv[1]);

	llama_log_set([](enum ggml_log_level level, const char *text, void * /* user_data */) {
		if (level < GGML_LOG_LEVEL_WARN) {
			return;
		}
		fprintf(stderr, "%s", text);
	}, nullptr);

#if TRACY_ENABLE
	tracy::StartupProfiler();
#endif

	// test1(model_path);
	test2(model_path);

#if TRACY_ENABLE
	tracy::ShutdownProfiler();
#endif
	// test_double_free(model_path);

	return 0;
}
