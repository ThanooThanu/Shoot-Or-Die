# Shoot-Or-Die

**Shoot-Or-Die** is a fast-paced first-person shooter where the player navigates a 6-layer maze gauntlet. Your goal is to sprint to the finish line while shooting barrels to breach obstacles, all while being chased by a relentless monster.

![Screenshot_1](https://github.com/user-attachments/assets/a5449557-d1df-4140-8510-9f062f71cc39)

## 🎮 Controls

* **W, A, S, D** : Movement
* **Left-Shift** : Sprint (Run faster)
* **Left-Click** : Shoot (Destroy barrels)
* **R** : Restart Game (When Dead or Won)
* **ESC** : Close Game

---

## 🛠️ Built With

* C++ (Standard 17)
* OpenGL (Core Profile)
* GLFW & GLAD
* GLM (Mathematics)
* Assimp (Model Loading)

---

## 📜 Credits & Acknowledgments

This project would not be possible without the following resources:

### Code & Framework
* **[LearnOpenGL](https://learnopengl.com/)** by Joey de Vries
    * The rendering engine, shader management, and model loading framework are based on the tutorials and code provided by LearnOpenGL.
    * **Note:** The helper classes in the `includes` folder (specifically `animator.h`, `model.h`, and `bone.h`) have been **modified and extended** for this project to support specific gameplay features (such as `GetFinalBoneMatrices` retrieval and custom animation logic).

### Libraries
* [GLFW](https://www.glfw.org/) - Window management
* [GLAD](https://glad.dav1d.de/) - OpenGL function loader
* [GLM](https://github.com/g-truc/glm) - OpenGL Mathematics
* [Assimp](https://github.com/assimp/assimp) - Open Asset Import Library
* [stb_image](https://github.com/nothings/stb) - Image loading library

### 3D Assets
* **Airgun Model:** "Haenel Model III-56" by [Dennis Haupt (3DHaupt.com)](https://sketchfab.com/3dhaupt) is licensed under Creative Commons Attribution.
* **Hunter Model:** [Mixamo / Adobe] (Characters and Animations)
* **Environment Textures:** Standard textures derived from LearnOpenGL resources.

---

### 📷 Gameplay Preview

https://github.com/user-attachments/assets/961991c6-aa09-46c7-a332-b1aca280f41c

<details>
<summary>View More Screenshots</summary>

<img width="1919" height="992" alt="Screenshot_2" src="https://github.com/user-attachments/assets/f6a57dcf-e9bc-47b2-85d1-6f1fd0fa04ac" />
<img width="1919" height="992" alt="Screenshot_3" src="https://github.com/user-attachments/assets/2fb2a8e3-39b4-4404-b2b3-ea63469bb0be" />

</details>
