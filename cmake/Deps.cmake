find_package(Boost REQUIRED COMPONENTS context filesystem)
find_package(SDL2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(kcp REQUIRED)
find_package(magic_enum REQUIRED)
find_package(nameof REQUIRED)
find_package(pareto REQUIRED)
find_package(replxx REQUIRED)
find_package(spdlog REQUIRED)
find_package(wasm3 REQUIRED)
find_package(Catch2 REQUIRED)
find_package(snappy REQUIRED)
find_package(brotli REQUIRED)
find_package(tinygltf REQUIRED)
find_package(Taskflow REQUIRED)
find_package(kissfft REQUIRED)
find_package(bgfx REQUIRED)
find_package(xxHash REQUIRED)
find_package(linalg REQUIRED)
find_package(miniaudio REQUIRED)
find_package(stb REQUIRED)
find_package(utf8.h REQUIRED)
find_package(pdqsort REQUIRED)

# add_library(imgui_club INTERFACE)
# target_include_directories(imgui_club INTERFACE imgui_club/imgui_memory_editor)

# set(imguizmo_SOURCES 
#   imguizmo/ImCurveEdit.cpp
#   imguizmo/ImGradient.cpp
#   imguizmo/ImGuizmo.cpp
#   imguizmo/ImSequencer.cpp
# )
# add_library(imguizmo STATIC ${imguizmo_SOURCES})
# target_include_directories(imguizmo PUBLIC imguizmo)
# target_include_directories(imguizmo PRIVATE bgfx/3rdparty/dear-imgui)
# target_link_libraries(imguizmo PUBLIC dear-imgui)

# add_library(implot STATIC 
#   implot/implot.cpp
#   implot/implot_items.cpp
# )
# target_include_directories(implot PUBLIC implot)
# target_link_libraries(implot
#   PUBLIC dear-imgui
#   PRIVATE stb
# )
