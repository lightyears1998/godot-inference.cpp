# llama.cpp Cheatsheet

Misc
llama_log_set

Lifetime
llama_backend_init <---> llama_backend_free
ggml_backend_load_all

Model
llama_model_default_params
n_gpu_layers, progress_callback
llama_model_load_from_file
llama_model_get_vocab
llama_model_free

Context
llama_context_default_params
n_ctx,
n_batch impact RAM usage
llama_init_from_model
llama_n_ctx(ctx)
llama_free

Sampler
llama_sampler_chain_init
llama_sampler_chain_add
llama_sampler_free

Chat
llama_model_chat_template

Memory and Generate
llama_get_memory(ctx)
llama_memory_seq_pos_max(memory, seq_id)
llama_tokenize(vocab, prompt, ...)
llama_decode(ctx, batch)
llama_sampler_sample
llama_vocab_is_eog
llama_token_to_piece
