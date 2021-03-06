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
      "src/parg.cpp",
      "third-party/xxHash/xxhash.c",
      "src/main.cpp",
    }

    includedirs {
      "third-party/stb/",
      "third-party/xxHash/",
      -- "third-party/LuaJIT/src/",
      -- "third-party/Handmade-Math/",
    }


    if os.get() == "windows" then
      -- nop
      defines { "QRED_PLATFORM_WINDOWS" }

      buildoptions {
        "-Wall",
        "-Wno-unknown-pragmas",
        "-Wno-unused-variable",
      }

      linkoptions { "-static" }

    elseif (os.get() == "macosx") then
      defines { "QRED_PLATFORM_MACOS" }

      buildoptions { "-Wall",
                     "-Werror",
                     "-fobjc-arc",
                     "-fobjc-arc",
                     "-stdlib=libc++",
                   }
      linkoptions {"-lstdc++"}
    else
      defines { "QRED_PLATFORM_LINUX" }

      buildoptions { "-Wall",
                     "-Werror",
                   }
      linkoptions {
        "-lstdc++",
        "-Wl,--no-as-needed -ldl",
        "-rdynamic"
      }

      links { "m" }

    end

    configuration "Debug"
       kind "ConsoleApp"
       flags { "Symbols" }
       flags { "FullSymbols" }

    configuration "Release"
       kind "WindowedApp"
       flags { "OptimizeSize" }

