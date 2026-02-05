#!/usr/bin/env python3
"""
Generate compile_commands.json for JUCE project.

This is needed because bear doesn't work well with Xcode's modern build system.
Run this script after making changes to the project structure or after
regenerating the Xcode project with Projucer.

Usage:
    python3 scripts/generate_compile_commands.py
"""

import json
import os
import subprocess
from pathlib import Path


def get_sdk_path():
    """Get the macOS SDK path using xcrun."""
    result = subprocess.run(
        ["xcrun", "--show-sdk-path"],
        capture_output=True,
        text=True,
        check=True
    )
    return result.stdout.strip()


def main():
    # Determine project directory (script is in scripts/)
    script_dir = Path(__file__).parent
    project_dir = script_dir.parent
    os.chdir(project_dir)

    juce_modules = Path.home() / "JUCE" / "modules"
    vst3_sdk = juce_modules / "juce_audio_processors_headless" / "format_types" / "VST3_SDK"
    macos_sdk = get_sdk_path()

    # Common compiler arguments
    common_args = [
        "clang++",
        "-std=c++17",
        "-stdlib=libc++",
        f"-isysroot{macos_sdk}",
        "-DDEBUG=1",
        "-D_DEBUG=1",
        "-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1",
        "-DJUCER_XCODE_MAC_F6D2F4CF=1",
        "-DJUCE_STANDALONE_APPLICATION=0",
        # JUCE module availability
        "-DJUCE_MODULE_AVAILABLE_juce_audio_basics=1",
        "-DJUCE_MODULE_AVAILABLE_juce_audio_devices=1",
        "-DJUCE_MODULE_AVAILABLE_juce_audio_formats=1",
        "-DJUCE_MODULE_AVAILABLE_juce_audio_plugin_client=1",
        "-DJUCE_MODULE_AVAILABLE_juce_audio_processors=1",
        "-DJUCE_MODULE_AVAILABLE_juce_audio_utils=1",
        "-DJUCE_MODULE_AVAILABLE_juce_core=1",
        "-DJUCE_MODULE_AVAILABLE_juce_data_structures=1",
        "-DJUCE_MODULE_AVAILABLE_juce_events=1",
        "-DJUCE_MODULE_AVAILABLE_juce_graphics=1",
        "-DJUCE_MODULE_AVAILABLE_juce_gui_basics=1",
        "-DJUCE_MODULE_AVAILABLE_juce_gui_extra=1",
        # Plugin definitions
        '-DJucePlugin_Name="GRAIN"',
        '-DJucePlugin_Desc="GRAIN"',
        '-DJucePlugin_Manufacturer="yourcompany"',
        '-DJucePlugin_Version=1.0.0',
        '-DJucePlugin_VersionString="1.0.0"',
        "-DJucePlugin_Build_VST3=1",
        "-DJucePlugin_Build_Standalone=0",
        "-DJucePlugin_Build_AU=0",
        "-DJucePlugin_Build_VST=0",
        "-DJucePlugin_IsSynth=0",
        "-DJucePlugin_WantsMidiInput=0",
        "-DJucePlugin_ProducesMidiOutput=0",
        "-DJucePlugin_IsMidiEffect=0",
        "-DJucePlugin_EditorRequiresKeyboardFocus=0",
        "-DJUCE_VST3_CAN_REPLACE_VST2=0",
        # Include paths
        f"-I{project_dir}/JuceLibraryCode",
        f"-I{juce_modules}",
        f"-I{vst3_sdk}",
        "-c"
    ]

    # Find all source files
    source_files = list(Path("Source").rglob("*.cpp"))

    entries = []
    for src in source_files:
        abs_path = str(project_dir / src)
        entries.append({
            "directory": str(project_dir),
            "file": abs_path,
            "arguments": common_args + [abs_path]
        })

    output_file = project_dir / "compile_commands.json"
    with open(output_file, "w") as f:
        json.dump(entries, f, indent=2)

    print(f"Generated {output_file} with {len(entries)} entries:")
    for entry in entries:
        print(f"  - {Path(entry['file']).name}")


if __name__ == "__main__":
    main()
