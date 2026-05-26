#pragma once

#include <thread>

inline std::thread::id main_thread_id {};

void library_terminate_handler();
