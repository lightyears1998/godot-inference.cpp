#pragma once

#include <thread>

class InferenceEngine;

inline std::thread::id main_thread_id {};
inline InferenceEngine* engine_instance = nullptr;

void library_terminate_handler();
