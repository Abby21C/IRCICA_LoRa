# WUR with Payload

Git to test SX1261 Wake Up with payload in signal

## Requirements  
- OS: Ubuntu or Windows (not tested in MacOS)
- [Visual Studio Code](https://code.visualstudio.com/download)
- [ESP-IDF v5.3.1](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md)  
- The project cloned from GitHub  
```git clone git@github.com:ecornejog/WUR-with-payload.git```


## Visual Studio Code    

To use Visual Studio Code properly, is recommended to use the next extensions:
 - C/C++ extension pack
 - Comment Anchors
 - Git Graph

## Structure of project:

```
├── components                 Folder where all the components are defined and coded
│   ├── ra01s
├── main
│   ├── CMakeLists.txt
|   ├── idf_component.yml
│   └── main.c                Main code of project
├── CMakeLists.txt
└── README.md                  This is the file you are currently reading
```

## Using ESP-IDF extension in VSCode

If all is installed properly, you'll able to see a task bar in the footer of Visual Studio Code, and in this task bar there are a lot of buttons, where you can Build, Flash, Monitor, Configure, etc.  
Follow this [link](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/basic_use.md) for more information about ESP-IDF.
