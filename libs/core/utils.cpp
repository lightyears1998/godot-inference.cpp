#include "utils.hpp"

#include <Windows.h>

std::string acp_to_utf8(const char *acp_str) {
	if (acp_str == nullptr) {
		return {};
	}

	int acp_len = static_cast<int>(strlen(acp_str));
	if (acp_len == 0) {
		return {};
	}

	int wide_len = MultiByteToWideChar(CP_ACP, 0, acp_str, acp_len, nullptr, 0);
	if (wide_len == 0) {
		return {};
	}

	std::wstring wide_str(wide_len, L'\0');
	MultiByteToWideChar(CP_ACP, 0, acp_str, acp_len, &wide_str[0], wide_len);

	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), wide_len, nullptr, 0, nullptr, nullptr);
	if (utf8_len == 0) {
		return {};
	}

	std::string utf8_str(utf8_len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), wide_len, &utf8_str[0], utf8_len, nullptr, nullptr);

	return utf8_str;
}
