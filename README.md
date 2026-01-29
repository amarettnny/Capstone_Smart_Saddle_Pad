# Capstone_Smart_Saddle_Pad

## Setting up ESP-IDF for the first time

- Follow the Espressif installation guide for your OS: https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/get-started/index.html, but clone v5.5.2
- Linux & MacOS:
    - Set up an alias to execute export.sh.
    - To locate your shell profile, run the following command:
      '''
      echo $Shell
      '''
    - for MacOS it should show:
      '''
      /bin/zsh
      '''
    - If so, then navigate to ~/.zshrc, then copy-paste the alias code on the documentation (replace esp-idf with your directory name (should be esp-idf-v5.5.2))
    - When you create a new terminal, it will automatically use ~/.zshrc to set environment variables. (Can also do source ~/.zshrc to re-run those variables)
    - Currently (as of 1/28/26): when you navigate to the esp-idf project, in your terminal, you will have to re-run:
      '''
      get_idf
      '''
