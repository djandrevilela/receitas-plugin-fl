cmake_minimum_required(VERSION 3.22)
project(Receitas VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()

option(RECEITAS_BUILD_PLUGIN "Compilar o plugin VST3 (descarrega o JUCE)" ON)
option(RECEITAS_COPY_AFTER_BUILD "Copiar o VST3 para a pasta de plugins do sistema após compilar" OFF)
option(RECEITAS_BUILD_RENDER_TEST "Compilar o teste de áudio offline (precisa do JUCE)" OFF)

set(RECEITAS_CORE_SOURCES core/Receitas.cpp)

# --- Motor + testes (sem JUCE) ---
add_library(receitas_core STATIC ${RECEITAS_CORE_SOURCES})
target_include_directories(receitas_core PUBLIC core)
add_executable(receitas_core_tests tests/core_tests.cpp)
target_link_libraries(receitas_core_tests PRIVATE receitas_core)
enable_testing()
add_test(NAME core COMMAND receitas_core_tests)

# --- Plugin VST3 ---
if(RECEITAS_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)

  juce_add_plugin(Receitas
    COMPANY_NAME "Harmonia Suite"
    PLUGIN_MANUFACTURER_CODE Hrml
    PLUGIN_CODE Rec1
    FORMATS VST3
    PRODUCT_NAME "Receitas"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${RECEITAS_COPY_AFTER_BUILD})

  target_sources(Receitas PRIVATE ${RECEITAS_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
  target_include_directories(Receitas PRIVATE core plugin)
  target_compile_definitions(Receitas PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(Receitas
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

  if(RECEITAS_BUILD_RENDER_TEST)
    juce_add_console_app(ReceitasRenderTest PRODUCT_NAME "ReceitasRenderTest")
    target_sources(ReceitasRenderTest PRIVATE ${RECEITAS_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp tests/render_test.cpp)
    target_include_directories(ReceitasRenderTest PRIVATE core plugin)
    target_compile_definitions(ReceitasRenderTest PRIVATE
      JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(ReceitasRenderTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
  endif()
endif()
