# Capstone_Smart_Saddle_Pad

## Setting up ESP-IDF for the first time

- This folder, along with its subfolders, contains the firmware that runs on the ESP32 microcontrollers using ESP-IDF. In order to compile and flash our ESP-IDF projects, you need to do the following:

    - Install the ESP-IDF extension on VS Code, and follow the ESP-IDF Setup to install the latest version of ESP-IDF (default parameters should be fine)
    - Open up the root folder of the ESP-IDF project in VS Code
    - Set the correct COM port, microcontroller, and ESP-IDF on the bottom left if necessary
    - Common Compilation Issues:
        - Make sure that the .vscode directory in the root folder of your project has a c_cpp_properties.json, launch.json, and a settings.json. If it is missing any of those, delete the .vscode folder, and have ESP-IDF generate it using the [Add .vscode subdirectoy files] button found by clicking on the ESP-IDF Explorer Icon in the Activity Bar and looking under Advanced.
        - Also see if deleting the build directory and rebuilding works as well.
        - Make sure that you configured the SDK to have the size of flash be 8MB for the XIAO ESP32C5. (change 2MB to 8 MB in CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y)

- **Linux & MacOS: (OPTIONAL)**
    - Set up an alias to execute export.sh.
    - To locate your shell profile, run the following command:
      ```
      echo $Shell
      ```
    - for MacOS it should show:
      ```
      /bin/zsh
      ```
    - If so, then navigate to *~/.zshrc*, then copy-paste the alias code on the documentation (replace *esp-idf* with your directory name (should be *esp-idf-v5.5.2*))
    - When you create a new terminal, it will automatically use *~/.zshrc* to set environment variables. (Can also do ```source ~/.zshrc``` to re-run those variables)
    - Currently (as of 1/28/26): when you navigate to the esp-idf project, in your terminal, you will have to re-run:
      ```
      get_idf
      ```
    - Then you will be able to do ```idf.py``` commands within your terminal. See the official ESP32 Guide for more commands: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-start-project.html 
