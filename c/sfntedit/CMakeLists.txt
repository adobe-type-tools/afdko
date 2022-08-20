add_library(sfntedit OBJECT
    sfntedit.cpp
)

set_property(TARGET sfntedit PROPERTY CXX_STANDARD 17)

target_include_directories(sfntedit PRIVATE ../spot/sfnt_includes ../shared/include)
