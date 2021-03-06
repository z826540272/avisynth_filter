#pragma once

#include "pch.h"


namespace AvsFilter {

class Registry {
public:
    Registry();
    virtual ~Registry();

    auto ReadString(const wchar_t *valueName) const -> std::wstring;
    auto ReadNumber(const wchar_t *valueName, int defaultValue) const -> DWORD;
    auto WriteString(const wchar_t *valueName, const std::wstring &valueString) const -> void;
    auto WriteNumber(const wchar_t *valueName, DWORD valueNumber) const -> void;

private:
    HKEY _registryKey;
};

}