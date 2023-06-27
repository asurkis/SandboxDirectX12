#pragma once

#include "pch.hpp"

class ShaderCompiler
{
    std::filesystem::path m_ShaderRoot;
    std::wstring          m_ShaderRootWStr;
    PDxcCompiler          m_Compiler;
    PDxcUtils             m_Utils;
    PDxcIncludeHandler    m_IncludeHandler;

    PBlob Compile(const wchar_t *filename, const wchar_t *target);

  public:
    explicit ShaderCompiler(std::filesystem::path shaderRoot);
    PBlob CompileVS(const wchar_t *filename) { return Compile(filename, L"vs_6_0"); }
    PBlob CompilePS(const wchar_t *filename) { return Compile(filename, L"ps_6_0"); }
};
