# godot-inference.cpp

🚧 Under heavy development: It's not ready yet!

A GDExtension that enables running generative models within Godot.
It is built on top of GGML, llama.cpp and whisper.cpp.

## When to use this library (and when not to)

- Give it a try for fun! :)
  Use it when you need to run an LLM or ASR model locally, optimize for a specific GGML backend, and existing tools like Ollama or LM Studio don’t meet your needs.
- Or when you want to build complex inference pipelines and make a lot of customizations.
- Or when you need to ship your fine-tuned model directly with the game binary. Since models are typically several gigabytes, the library’s added file size is negligible in comparison.

I've done my best to make it reliable, but for serious production work, check out the alternatives below.

### Alternatives

- Have users set up a local API or use a web API. This may be a better approach, as it reduces the game’s distribution size and lets users customize model parameters.
- [LLamaSharp](https://github.com/SciSharp/LLamaSharp) may be a better fit for production use.
