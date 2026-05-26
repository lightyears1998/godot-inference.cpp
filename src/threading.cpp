#include "threading.hpp"

#include "globals.hpp"

#include <thread>

using namespace std;

bool is_main_thread(){
	return this_thread::get_id() == main_thread_id;
}
