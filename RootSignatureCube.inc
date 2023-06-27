#ifndef ROOT_SIGNATURE_CUBE
#define ROOT_SIGNATURE_CUBE                                                          \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT                                  " \
    "        | DENY_HULL_SHADER_ROOT_ACCESS                                        " \
    "        | DENY_DOMAIN_SHADER_ROOT_ACCESS                                      " \
    "        | DENY_GEOMETRY_SHADER_ROOT_ACCESS                                    " \
    "        | DENY_PIXEL_SHADER_ROOT_ACCESS),                                     " \
    "RootConstants(num32BitConstants=16, b0, visibility=SHADER_VISIBILITY_VERTEX), "
#endif
