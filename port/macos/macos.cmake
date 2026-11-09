# macOS: the backend on AppKit (windows, input, the menu bar, native dialog
# controls), Core Audio for the sound effects and its MIDI synthesizer for the
# music, and optionally an app bundle (-DLEMMINGS_MACOS_BUNDLE=ON:
# build/native/Lemmings.app; put the game's WINLEMM folder into
# Lemmings.app/Contents/Resources, or start it with --data). Included by
# CMakeLists.txt.

option(LEMMINGS_MACOS_BUNDLE "Build Lemmings.app instead of a plain executable" OFF)

set(COCOA_SRC ${CMAKE_SOURCE_DIR}/port/macos/plat_cocoa.m ${CMAKE_SOURCE_DIR}/port/macos/menu_cocoa.m
              ${CMAKE_SOURCE_DIR}/port/macos/controls_cocoa.m)
target_sources(winlayer PRIVATE ${COCOA_SRC} ${CMAKE_SOURCE_DIR}/port/macos/audio_coreaudio.c
                                ${CMAKE_SOURCE_DIR}/port/macos/midi_coreaudio.c)
set_source_files_properties(${COCOA_SRC} PROPERTIES COMPILE_OPTIONS -fobjc-arc)
target_include_directories(winlayer PRIVATE ${CMAKE_SOURCE_DIR}/port/src)
target_link_libraries(winlayer PUBLIC "-framework AppKit" "-framework QuartzCore" "-framework CoreGraphics"
                                      "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation")

function(lemmings_macos_target target)
  if(LEMMINGS_MACOS_BUNDLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE ON
      OUTPUT_NAME Lemmings
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/port/macos/Info.plist.in
      MACOSX_BUNDLE_BUNDLE_NAME Lemmings
      MACOSX_BUNDLE_GUI_IDENTIFIER io.github.nitrosgits.lemmings-win95
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    # ad-hoc signed, as Apple silicon wants every executable signed
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND codesign --force --sign - $<TARGET_FILE:${target}>
      VERBATIM)
  endif()
endfunction()
