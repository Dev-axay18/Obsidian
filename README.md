<!-- Banner -->
<p align="center">
  <img src="https://github.com/Dev-axay18/Obsidian/blob/main/assets/1.gif?raw=true" alt="Obsidian Banner" width="100%" />
</p>

<!-- Logo -->
<p align="center">
  <img src="https://github.com/Dev-axay18/Obsidian/blob/main/assets/removed-background%20(logo).png?raw=true" alt="Obsidian Logo" width="210" />
</p>

<h1 align="center">Obsidian OS</h1>

<p align="center">
  <i>A minimalist, bootable, low-level operating system written in pure C and Assembly.</i>  
</p>

<p align="center">
  <img src="https://img.shields.io/badge/architecture-x86-informational?style=flat&logo=linux" />
  <img src="https://img.shields.io/badge/language-C%20%26%20ASM-blue?style=flat&logo=c" />
  <img src="https://img.shields.io/badge/bootable-yes-green?style=flat&logo=gnu" />
  <img src="https://img.shields.io/badge/build-pure%20makefile-orange?style=flat&logo=make" />
</p>

---

<blockquote>
  <strong style="color: red;">
    ⚠️ WARNING: DO NOT TRY THIS PROJECT ON YOUR MACHINE UNLESS YOU HAVE PROPER KNOWLEDGE OF ASSEMBLY PROGRAMMING.
  </strong><br/>
  
  <span>This project involves low-level operations that can affect your system’s stability or boot process.</span>
</blockquote>


---

## 🧠 About the Project

> Obsidian is a lightweight kernel designed to demystify how operating systems work under the hood. From custom bootloaders to basic CLI handling, this project dives into bare-metal programming.

---

## 🚀 Demo Preview

<p align="center">
  <img src="https://i.imgur.com/yourPreviewGif.gif" alt="Obsidian Boot Demo" width="80%" />
</p>

---

## ⚙️ Features

- 🧠 Bootable from BIOS (via QEMU / VirtualBox)
- ⚙️ Custom kernel with text interface
- 🔌 x86 Interrupt Handling & ISRs
- 💾 Minimal binary footprint
- 🛠️ NASM + C integration
- 🧪 Pure Makefile build system

---

## 🛠️ Tech Stack

Languages:
  - C
  - Assembly (NASM)

Tools:
  - QEMU (Virtualization)
  - GNU Make
  - x86 BIOS Bootloader


---

## 📂 Project Structure

```
📁 Obsidian/
├── 🔧 bootloader/       # Real-mode boot code
├── 💻 src/              # Kernel source (C/ASM)
├── 📐 include/          # Header files
├── 🗃️ bin/              # Compiled binaries
├── 🧰 Makefile          # Build script
└── 📄 README.md         # You are here
```

---

## 🧪 Build & Run Locally

 ### Clone the repo
```bash
git clone https://github.com/Dev-axay18/Obsidian
cd Obsidian
```
### Build kernel and boot
```bash
make
qemu-system-x86_64 -kernel bin/obsidian.bin
```

✅ Requires `qemu`, `nasm`, and `make` installed on your system.

---

## 📈 Roadmap

* [x] BIOS-based boot
* [x] Simple kernel entry
* [x] Custom screen printing functions
* [ ] Input handling
* [ ] File system support
* [ ] Shell with basic commands
* [ ] Multithreading
* [ ] Networking stack (experimental)

---

## 📜 License

This project is licensed under the **MIT License**.
Feel free to use, fork, modify — just give credit where it’s due ❤️

---

## 🙌 Credits & Inspirations

* [OSDev.org](https://wiki.osdev.org/Main_Page)
* [Tanenbaum’s Modern Operating Systems](https://www.pearson.com/en-us/subject-catalog/p/modern-operating-systems/P200000003805/9780133591620)
* [Programology](https://www.youtube.com/watch?v=SL--qoiu7yA&list=PLR2FqYUVaFJpHPw1ExSVJZFNlXzJYGAT1)


---

---


