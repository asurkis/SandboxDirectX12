#define ROOT_SIGNATURE_SPONZA                                                                                          \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT                                  "                                   \
    "        | DENY_HULL_SHADER_ROOT_ACCESS                                        "                                   \
    "        | DENY_DOMAIN_SHADER_ROOT_ACCESS                                      "                                   \
    "        | DENY_GEOMETRY_SHADER_ROOT_ACCESS),                                  "                                   \
    "RootConstants(b0, num32BitConstants=16, visibility=SHADER_VISIBILITY_VERTEX), "                                   \
    "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),                 "                                   \
    "StaticSampler(s0,                                                             "                                   \
    "    filter = FILTER_ANISOTROPIC,                                              "                                   \
    "    addressU = TEXTURE_ADDRESS_WRAP,                                          "                                    \
    "    addressV = TEXTURE_ADDRESS_WRAP,                                          "                                    \
    "    addressW = TEXTURE_ADDRESS_WRAP)                                          "
