juce_add_gui_app(DrumBoxStandalone
    PRODUCT_NAME "DrumBox Standalone"
)

juce_generate_juce_header(DrumBoxStandalone)

target_sources(DrumBoxStandalone PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
)

target_link_libraries(DrumBoxStandalone PRIVATE
    drumbox_core
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_lto_flags
)

target_compile_definitions(DrumBoxStandalone PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
