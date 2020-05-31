solution "qred_tp"
  configurations { "Debug", "Release" }
  location "build"
  targetdir "bin"
  debugdir "./runtime/"
  platforms { "x64" }
  flags { "NoExceptions",
          "NoRTTI",
          "FatalWarnings",
          "StaticRuntime" }
  -- premake.gcc.cc  = "clang"
  -- premake.gcc.cxx = "clang++"
  -- premake.gcc.ar  = "ar"

  configuration "Debug"
    defines { "DEBUG" }
    flags { "FullSymbols" }

  configuration "Release"
    defines { "NDEBUG" }
    flags { "OptimizeSize" }

  -- App project
  -- ============
  project "qred_tp"
    language "C"
    files {
      "src/parg.c",
      "src/main.cpp",
    }

    includedirs {
      "third-party/stb/",
      -- "third-party/sokol/util",
      -- "third-party/LuaJIT/src/",
      -- "third-party/Handmade-Math/",
    }

    links { }

    if os.get() == "windows" then
      -- nop
      defines { "QRED_PLATFORM_WINDOWS" }

      buildoptions {
        "-Wall",
        "-std=c++11",
        "-Wno-unknown-pragmas",
        "-Wno-unused-variable",
        "-fno-threadsafe-statics"
      }

    elseif (os.get() == "macosx") then
      defines { "QRED_PLATFORM_MACOS" }

      buildoptions { "-Wall",
                     "-Werror",
                     "-Wno-invalid-offsetof",
                     "-fobjc-arc",
                     "-fobjc-arc",
                     "-stdlib=libc++",
                   }
      linkoptions {"-lstdc++"}
    else
      defines { "QRED_PLATFORM_LINUX" }

      buildoptions { "-Wall",
                     "-Werror",
                     "-Wno-invalid-offsetof",
                   }
      linkoptions {
        "-lstdc++",
        "-Wl,--no-as-needed -ldl",
        "-rdynamic"
      }

    end

    configuration "Debug"
       kind "ConsoleApp"
       flags { "Symbols" }
       flags { "FullSymbols" }

    configuration "Release"
       kind "WindowedApp"
       flags { "OptimizeSize" }

