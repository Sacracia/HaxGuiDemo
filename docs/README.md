# HaxGui
HaxGui is a lightweight immediate-mode graphics library tailored for creating in-game overlays. Unlike popular alternatives that focus on rapid prototyping with predefined styles, HaxGui prioritizes complete creative control. The library intentionally avoids providing "cookie-cutter" widgets. Instead, it equips developers with a robust low-level API to build entirely custom interfaces from the ground up, ensuring the UI perfectly matches the desired look and feel.

**Platform:** Windows

**Supported backends:** DirectX 11

<p align="center">
  <img src="https://i.imgur.com/1rlpf9i.gif">
</p>

---

### Features
* **NO external dependencies**, utilizes native Windows libraries instead of bulky open-source files
* **Written in C++20 code**
* **High-quality SDF shapes rendering**
* **Simple data-oriented architecture**
* **Custom lightweight classes** designed to be faster and more performant than STL
* **Widgets state keeping system**
* **Multiple Fonts formats supported** (TTF, OTF, WOFF, WOFF2, Arfont (MSDF))
* **Windows system fonts supported** (Arial, Segoe UI...)
* **Pure low-level API** without bloated widgets — build your own custom UI from scratch
* **Free and open source** under the MIT license
---

### Integration
The library is designed to be easily dropped into project. Just add the hax*.h and hax*.cpp files. The API documentation is available in docs/API.md, with practical integration and UI creation example provided in the hax_gui_demo.cpp file

---

### Third-party projects
Below are some projects using **HaxGui**:

<p align="center">
  <img src="https://i.imgur.com/0NQhuRA.png" alt="RepoHax Logo">
  <br>
  <a href="https://github.com/Sacracia/RepoHax">https://github.com/Sacracia/RepoHax</a>
</p>


### License
HaxGui is licensed under the MIT License.