#define ROOT_SIGNATURE_2                                                                                               \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT                                "                                     \
    "        | DENY_HULL_SHADER_ROOT_ACCESS                                      "                                     \
    "        | DENY_DOMAIN_SHADER_ROOT_ACCESS                                    "                                     \
    "        | DENY_GEOMETRY_SHADER_ROOT_ACCESS),                                "                                     \
    "RootConstants(num32BitConstants=2, b0, visibility=SHADER_VISIBILITY_PIXEL), "                                     \
    "DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),               "
