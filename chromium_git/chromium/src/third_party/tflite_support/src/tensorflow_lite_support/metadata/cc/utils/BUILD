package(
    default_visibility = [
        "//tensorflow_lite_support:internal",
    ],
    licenses = ["notice"],  # Apache 2.0
)

cc_library(
    name = "zip_mem_file",
    srcs = ["zip_mem_file.cc"],
    hdrs = ["zip_mem_file.h"],
    deps = [
        "@com_google_absl//absl/strings",
        "@zlib//:zlib_minizip",
    ],
)
