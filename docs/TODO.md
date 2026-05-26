# TODO

## Architecture

- [ ] Consider static linking of 3rd party libs. 
- [ ] Try RAG. 
  - [ ] USearch / Faiss: I dont want to use GPU acceleration since LLM Model has taken a lot VRAM, so perhaps usearch is better.

## Refactoring

- [ ] Support multiple models loading.


```txt
248068 <think> 
248069 </think>
LLAMA_TOKEN_ATTR_USER_DEFINED
not special, can be tokenized
should enforce a safer way.
set attr to prevent tokenizer tokenize these special api.
```
