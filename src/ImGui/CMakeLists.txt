set(headers
    "imconfig.h"
    "imgui.h"
    "imgui_impl_glfw.h"
    "imgui_impl_opengl3.h"
	"imgui_impl_opengl3_loader.h"
    "imgui_internal.h"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h"
    )

set(sources
    "imgui.cpp"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_impl_glfw.cpp"
    "imgui_impl_opengl3.cpp"
	"imgui_tables.cpp"
	"imgui_widgets.cpp"
    )

list(SORT headers)
list(SORT sources)

source_group(Headers FILES ${headers})
source_group(Sources FILES ${sources})

cuda_add_library(imGui ${sources} ${headers})
