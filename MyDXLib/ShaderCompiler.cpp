#include "ShaderCompiler.hpp"
#include "Utils.hpp"

ShaderCompiler::ShaderCompiler(std::filesystem::path shaderRoot)
    : m_ShaderRoot(std::move(shaderRoot)),
      m_ShaderRootWStr(m_ShaderRoot)
{
    Assert(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_Compiler.ReleaseAndGetAddressOf())));
    Assert(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_Utils.ReleaseAndGetAddressOf())));
    Assert(m_Utils->CreateDefaultIncludeHandler(m_IncludeHandler.ReleaseAndGetAddressOf()));
}

PBlob ShaderCompiler::Compile(const wchar_t *filename, const wchar_t *target)
{
    std::vector<char> contents;
    {
        std::ifstream fin(m_ShaderRoot / filename, std::ios::binary);
        fin.ignore((std::numeric_limits<std::streamsize>::max)());
        std::streamsize size = fin.gcount();
        contents.resize(size);
        fin.seekg(0);
        fin.read(contents.data(), contents.size());
    }

    ComPtr<IDxcBlobEncoding> sourceBlob;
    Assert(m_Utils->CreateBlob(contents.data(), contents.size(), CP_UTF8, sourceBlob.ReleaseAndGetAddressOf()));

    std::vector<LPCWSTR> args;
    args.push_back(L"-T");
    args.push_back(target);
    args.push_back(L"-I");
    args.push_back(m_ShaderRootWStr.c_str());

#ifdef _DEBUG
    args.push_back(DXC_ARG_DEBUG);
    args.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
#endif

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr       = sourceBlob->GetBufferPointer();
    sourceBuffer.Size      = sourceBlob->GetBufferSize();

    ComPtr<IDxcResult> compileResult;
    Assert(m_Compiler->Compile(&sourceBuffer,
                               args.data(),
                               args.size(),
                               m_IncludeHandler.Get(),
                               IID_PPV_ARGS(compileResult.ReleaseAndGetAddressOf())));

    ComPtr<IDxcBlobUtf8>  errors;
    ComPtr<IDxcBlobUtf16> name;
    (compileResult->GetOutput(
        DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), name.ReleaseAndGetAddressOf()));
    if (errors && errors->GetBufferSize() > 0)
        throw std::runtime_error(static_cast<char *>(errors->GetBufferPointer()));

    PBlob object;
    compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), name.ReleaseAndGetAddressOf());
    return object;
}
