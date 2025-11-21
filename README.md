# **OpenGL Rendering Engine**

A C++ OpenGL rendering project featuring Phong lighting, 3D model loading, and an ImGui interface.

## **🛠 Dependencies**

This project manages dependencies automatically using **vcpkg Manifest Mode**.

* **OpenGL/GLFW/GLAD**: Graphics and Windowing  
* **GLM**: Mathematics  
* **Assimp**: 3D Model Loading  
* **ImGui**: User Interface  
* **stb\_image**: Texture Loading

## **⚙️ Prerequisites**

Before running this project, ensure you have the following installed:

1. **Visual Studio 2019 or 2022** (with "Desktop development with C++" workload).  
2. **Git**.  
3. **vcpkg** (The C++ Package Manager).

### **One-Time vcpkg Setup**

If you have never used vcpkg on your machine before, follow these steps (you only need to do this once in your life):

1. Clone vcpkg (e.g., to your C: drive):  
   git clone \[https://github.com/Microsoft/vcpkg.git\](https://github.com/Microsoft/vcpkg.git) C:\\vcpkg

2. Bootstrap vcpkg:  
   .\\C:\\vcpkg\\bootstrap-vcpkg.bat

3. **Crucial Step:** Integrate it with Visual Studio so it can read the manifest file:  
   .\\C:\\vcpkg\\vcpkg integrate install

## **🚀 Quick Start**

Once the prerequisites are met, running the project is simple:

1. **Clone the repository:**  
   git clone https://github.com/hirentg/Project3\_OpenGL

2. **Open the Project:**  
   * Navigate to the folder and double-click the .sln file to open it in **Visual Studio**.  
3. **Build:**  
   * Select your configuration (e.g., **Debug** or **Release**) and Platform (**x64**).  
   * Press **Ctrl \+ Shift \+ B** (or right-click the project and select **Build**).

**Note:** The first time you build, Visual Studio will detect the vcpkg.json file and **automatically download and compile** Assimp, ImGui, and other dependencies. This might take a few minutes. Look at the "Output" window to see the progress.

4. **Run:**  
   * Once the build is complete, press **F5** to run the application.

## **🎮 Controls**

* **WASD**: Move Camera  
* **Mouse**: Look around  
* **TAB**: Open ImGUI menu  
* **ESC**: Close Window / Unlock Mouse

## **❓ Troubleshooting**

**"Cannot open include file 'assimp/scene.h'"**

* This means vcpkg didn't run. Ensure you ran vcpkg integrate install in your terminal.  
* Right-click the project in Visual Studio \-\> **Properties**. Ensure **vcpkg** is enabled in the configuration properties.

**Build seems stuck?**

* Check the **Output** window (not the Error List). vcpkg is likely compiling the libraries in the background. This only happens on the very first build.