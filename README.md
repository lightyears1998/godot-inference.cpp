# Godot llama.cpp

A GDExtension that enables running llama.cpp within Godot.
It serves as a thin wrapper around llama.cpp and allows GDScript to access a high-level LLM API.

## When to use this library and alternatives

- Give it a try for fun! :)
- I've done my best to make it reliable, but for serious production work, check out the alternatives below.

Alternatives:

- Have users set up a local LLM API or use a web API. This may be a better approach, as it reduces the game’s distribution size and lets users customize model parameters.
- [LLamaSharp](https://github.com/SciSharp/LLamaSharp) may be a better fit for production use.
