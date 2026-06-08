# Benchmark

```text 
40t/s 
# reasoning general task
.\llama-cli.exe -m .\models\Qwen3.5-4B-Q4_K_M-Tuned.gguf --temperature 1.0 --top-p 0.95 --top-k 20 --min-p 0.0 --presence-penalty 1.5 --repeat-penalty 1.0 -ctk q8_0 -ctv q8_0 --ctx-size 32768

// .\llama-server.exe -m .\models\Qwen3.5-4B-Q4_K_M.gguf --reasoning off -ctk q8_0 -ctv q8_0
// --ctx-size 16384
// --ctx-size 32768 40t/s Best performance
// --ctx-size 65536 [28..10]t/s Performance will gradually degrade as VRAM runs out.
// --ctx-size 131072(2^17) 262144(2^18, MAX)

.\llama-cli.exe -m .\models\Qwen3.5-4B-Q4_K_M-Tuned.gguf --reasoning off --temperature 0.7 --top-p 0.8 --top-k 20 --min-p 0.0 --presence-penalty 1.5 --repeat-penalty 1.0 -ctk q8_0 -ctv q8_0 --ctx-size 32768
```
