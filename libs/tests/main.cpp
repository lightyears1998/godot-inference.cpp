#include "core/utils.hpp"

#include <fmt/format.h>
#include <llama.h>
#include <ggml.h>
#include <clocale>
#include <string>
#include <vector>
#include <iostream>

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

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = 26210;
	ctx_params.n_batch = 2048;

	llama_context *ctx = llama_init_from_model(model, ctx_params);
	if (!ctx) {
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return true;
	}

	// initialize the sampler
	auto sampler_params = llama_sampler_chain_default_params();
	llama_sampler *smpl = llama_sampler_chain_init(sampler_params);
	llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
	llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
	llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

	std::string test = "<think></think>This is a bad apple, and that too.";
	std::vector<llama_token> tokens(1024);
	llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), true, true);
	llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), true, false);
	llama_tokenize(vocab, test.c_str(), test.size(), tokens.data(), tokens.size(), false, false);

	for (auto token : tokens) {
		auto attr = llama_token_get_attr(vocab, token);
		std::puts(std::to_string(attr).c_str());
	}

	// helper function to evaluate a prompt and generate a response
	auto generate = [&](const std::string &prompt) {
		std::string response;

		const bool is_first = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) == -1;

		// tokenize the prompt
		const int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
		std::vector<llama_token> prompt_tokens(n_prompt_tokens);
		if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0) {
			GGML_ABORT("failed to tokenize the prompt\n");
		}

		// prepare a batch for the prompt
		llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
		llama_token new_token_id;
		while (true) {
			// check if we have enough space in the context to evaluate this batch
			int n_ctx = llama_n_ctx(ctx);
			int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
			if (n_ctx_used + batch.n_tokens > n_ctx) {
				printf("\033[0m\n");
				fprintf(stderr, "context size exceeded\n");
				exit(0);
			}

			int ret = llama_decode(ctx, batch);
			if (ret != 0) {
				GGML_ABORT("failed to decode, ret = %d\n", ret);
			}

			// sample the next token
			new_token_id = llama_sampler_sample(smpl, ctx, -1);

			// is it an end of generation?
			if (llama_vocab_is_eog(vocab, new_token_id)) {
				break;
			}

			// convert the token to a string, print it and add it to the response
			char buf[256];
			int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
			if (n < 0) {
				GGML_ABORT("failed to convert token to piece\n");
			}
			std::string piece(buf, n);
			printf("%s", piece.c_str());
			fflush(stdout);
			response += piece;

			// prepare the next batch with the sampled token
			batch = llama_batch_get_one(&new_token_id, 1);
		}

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
		fprintf(stderr, "%s", text);
	}, nullptr);

	test1(model_path);
	// test_double_free(model_path);

	return 0;
}
