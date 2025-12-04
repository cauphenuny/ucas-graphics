add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")
add_requires("opengl", {system = true})
add_requires("glut", {system = true})
add_requires("glfw3", {system = true})
add_requires("spdlog", {system = true})
add_requires("magic_enum")

target("project1")
    set_kind("binary")
    set_warnings("all")
    add_files("src/basics/*.cpp")
    add_packages("opengl")
    add_packages("glut")
    add_packages("spdlog")
    add_packages("glfw3")
    add_packages("magic_enum")
    if is_plat("macosx") then
        add_frameworks("Cocoa", "CoreFoundation", "IOKit")
    end

target("meshark")
    set_kind("static")
    add_files("src/mesh/meshark/src/*.cc")
    add_includedirs("src/mesh/meshark/include", {public = true})
    add_includedirs("src/mesh/external/glm", {public = true})
    add_headerfiles("src/mesh/meshark/include/(**.h)")

target("project2")
    set_kind("binary")
    add_files("src/mesh/meshark/apps/simplify.cc")
    add_deps("meshark")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

