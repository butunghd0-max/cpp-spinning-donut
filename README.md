# 🍩 Spinning Donut in C++

A 3D spinning donut (torus) rendered entirely in the terminal using ASCII characters and ANSI color codes. No graphics libraries, no GPU, no external dependencies — just raw math, trigonometry, and `stdout`.

This project is heavily inspired by [**Andy Sloane's original `donut.c`**](https://www.a1k0n.net/2011/07/20/donut-math.html) (2006/2011) and the viral [**"Donut-shaped C code that generates a 3D spinning donut"**](https://www.youtube.com/watch?v=DEqXNfs_HhY) phenomenon. I first discovered this through the [**Joma Tech video**](https://www.youtube.com/watch?v=74FJ8TTMM5E) and thought it was one of the hardest, most mind-bending things I'd ever seen — so I had to try it myself.

This version adds **ANSI 256-color gradients**, **flicker-free single-write rendering**, and **delta-time rotation** on top of the original concept.

---

## Table of Contents

- [Demo](#demo)
- [How It Works — The Math](#how-it-works--the-math)
  - [1. Building the Torus](#1-building-the-torus)
  - [2. Spinning It (3D Rotation Matrices)](#2-spinning-it-3d-rotation-matrices)
  - [3. Perspective Projection (3D → 2D)](#3-perspective-projection-3d--2d)
  - [4. Z-Buffering](#4-z-buffering)
  - [5. Illumination & ASCII Shading](#5-illumination--ascii-shading)
  - [6. ANSI Color Gradient](#6-ansi-color-gradient)
- [Constants & Parameters](#constants--parameters)
- [Build & Run](#build--run)
- [References & Sources](#references--sources)

---

## Demo

```
                 .::;;;;;;!!*****!!;;::~
             ,:;=!**##$$@@@@@@@@$$##**!=;:,
          ,-:;=!*#$$@@@@@@@@@@@@@@@@$$#*!=;:-,
        .~:;=!*#$@@@@@@@@@@@@@@@@@@@@@@$#*!=;:~.
      .-:;=!*#$@@@@@@@@@@@@@@@@@@@@@@@@@$#*!=;:-.
     ,~:;=*#$@@@@@@@@@@@@@@@@@@@@@@@@@@@@$#*=;:~,
    ,-:;=!#$@@@@@@@@@@@@@@@@@@@@@@@@@@@@@$#!=;:-,
    ~:;=!*$@@@@@@@@@@@@@@@@@@         @@@@$*!=;:~
   ,:;=!*#@@@@@@@@@@@@@@                @@@#*!=;:,
   ~:;=!*$@@@@@@@@@@@                   @@$*!=;:~
   :;=!*#$@@@@@@@@@@                    @$#*!=;:
   :;=!*#$@@@@@@@@@                     $#*!=;:~
   :;=!*#$@@@@@@@@@                     $#*!=;:
   ~;=!*#$@@@@@@@@@                    @$#*!=;:
   ,:;=!*#@@@@@@@@@@                  @@#*!=;:,
    ~:;=!*$@@@@@@@@@@@@             @@@$*!=;:~
    ,-:;=!#$@@@@@@@@@@@@@@@@@@@@@@@@@@$#!=;:-,
     ,~:;=*#$@@@@@@@@@@@@@@@@@@@@@@@@$#*=;:~,
       -:;=!*#$@@@@@@@@@@@@@@@@@@@@$#*!=;:-
        .~:;=!*#$@@@@@@@@@@@@@@@@$#*!=;:~.
          ,-:;=!*#$$@@@@@@@@@@$$#*!=;:-,
             ,:;=!**##$$$$$$##**!=;:,
                 .::;;;;;;!!;;::~
```

> The actual output is rendered with a navy → teal → cyan → white ANSI color gradient in your terminal.

---

## How It Works — The Math

This is genuinely one of the most mathematically dense small programs you'll encounter. Every single frame, it computes thousands of 3D points, rotates them in space, projects them onto a flat screen, and calculates lighting — all with nothing but `sin()`, `cos()`, and basic arithmetic.

### 1. Building the Torus

A donut is mathematically called a **torus**. You construct one by taking a small circle of radius `R1` and sweeping it around a larger circle of radius `R2`:

- **θ (theta)** sweeps around the small cross-section circle (0 → 2π)
- **φ (phi)** sweeps that circle around the central axis (0 → 2π)

The parametric equations for a point on the torus surface:

```
x = (R2 + R1·cos θ) · cos φ
y = (R2 + R1·cos θ) · sin φ
z = R1 · sin θ
```

In the code, `R1 = 1` and `R2 = 2`, meaning the tube radius is 1 and the center-to-tube distance is 2.

### 2. Spinning It (3D Rotation Matrices)

To animate the donut, we rotate every point around the **X-axis** by angle `A` and the **Z-axis** by angle `B`. This is done by multiplying the `(x, y, z)` coordinates by standard 3D rotation matrices.

The rotation about the X-axis by angle A:

```
         ⎡ 1    0       0   ⎤
Rx(A) =  ⎢ 0   cos A  -sin A⎥
         ⎣ 0   sin A   cos A⎦
```

The rotation about the Z-axis by angle B:

```
         ⎡ cos B  -sin B   0 ⎤
Rz(B) =  ⎢ sin B   cos B   0 ⎥
         ⎣   0       0     1 ⎦
```

Rather than doing full matrix multiplications at runtime, the code pre-computes `sinA`, `cosA`, `sinB`, `cosB` and multiplies them out algebraically for each point — saving a ton of operations per frame.

### 3. Perspective Projection (3D → 2D)

The terminal screen is flat, so we need to project 3D points onto 2D. This uses **perspective division** — objects further away appear smaller:

```
x' = (K1 · x) / (z + K2)
y' = (K1 · y) / (z + K2)
```

Where:

- **K1** = projection scale factor (how large the donut appears on screen)
- **K2** = distance from the viewer to the donut (set to `5`)

The code computes `invZ = 1 / (z + K2)` once and reuses it, which is cheaper than dividing twice.

### 4. Z-Buffering

Multiple 3D points can project to the same `(x', y')` screen position. To handle this, a **z-buffer** stores the depth (`invZ`) of whatever is currently drawn at each pixel. Before drawing a new point, the code checks:

```cpp
if (invZ > zbuf[idx])  // is this point closer to the viewer?
```

If yes, it overwrites the pixel. If no, it's hidden behind something already drawn. This is the same fundamental technique used by GPUs in modern 3D games.

### 5. Illumination & ASCII Shading

To simulate lighting, the code computes a **surface normal** for each point on the torus (the direction perpendicular to the surface). It then takes the **dot product** of the normal with a fixed light direction — this gives the cosine of the angle between them:

- **Dot product > 0** → surface faces the light → bright
- **Dot product ≤ 0** → surface faces away → dark / not drawn

The resulting luminance value `N` indexes into an ASCII brightness ramp:

```
.,-~:;=!*#$@
```

Where `.` is the dimmest and `@` is the brightest. This single string is the entire "shader" of the renderer.

### 6. ANSI Color Gradient

On top of the ASCII luminance, this version maps each brightness level to an **ANSI 256-color escape code**, creating a smooth gradient:

| Luminance | Character | Color       | ANSI Code  |
| --------- | --------- | ----------- | ---------- |
| 0         | `.`       | Dark Navy   | `38;5;17`  |
| 1         | `,`       | Navy        | `38;5;18`  |
| 2         | `-`       | Deep Blue   | `38;5;19`  |
| 3         | `~`       | Steel Blue  | `38;5;25`  |
| 4         | `:`       | Blue        | `38;5;31`  |
| 5         | `;`       | Teal        | `38;5;37`  |
| 6         | `=`       | Cyan        | `38;5;43`  |
| 7         | `!`       | Aqua        | `38;5;79`  |
| 8         | `*`       | Light Green | `38;5;115` |
| 9         | `#`       | Pale Mint   | `38;5;151` |
| 10        | `$`       | Near-White  | `38;5;188` |
| 11        | `@`       | Pure White  | `38;5;231` |

The entire frame is built into a single `std::string` and flushed to `stdout` in one write, eliminating the flicker that the original `putchar`-per-character approach suffers from.

---

## Constants & Parameters

| Constant     | Value                | Purpose                                                                  |
| ------------ | -------------------- | ------------------------------------------------------------------------ |
| `R1`         | `1.0`                | Tube (cross-section) radius                                              |
| `R2`         | `2.0`                | Distance from origin to the center of the tube                           |
| `K2`         | `5.0`                | Distance from the viewer to the donut                                    |
| `K1`         | `(H-4)·K2 / (R1+R2)` | Projection scale — derived from terminal height so the donut always fits |
| `THETA_STEP` | `0.04`               | Angular resolution for the cross-section                                 |
| `PHI_STEP`   | `0.02`               | Angular resolution for the sweep around the axis                         |
| `WIDTH`      | `80`                 | Terminal columns                                                         |
| `HEIGHT`     | `24`                 | Terminal rows                                                            |

---

## Build & Run

### Prerequisites

- A C++ compiler with C++11 or later support (e.g., `g++`, `clang++`, `MSVC`)
- A terminal that supports ANSI escape codes (VS Code terminal, Windows Terminal, most Linux/macOS terminals)

### Compile & Run

```bash
g++ -O2 main.cpp -o donut.exe
./donut.exe
```

Press **Ctrl+C** to stop.

> **Note:** The old `cmd.exe` on Windows does not support ANSI escape codes. Use **Windows Terminal** or the **VS Code integrated terminal** instead.

---

## References & Sources

- Sloane, Andy. "Donut Math: How Donut.c Works." _a1k0n.net_, 20 Jul. 2011, [www.a1k0n.net/2011/07/20/donut-math.html](https://www.a1k0n.net/2011/07/20/donut-math.html). The original, comprehensive mathematical breakdown of the `donut.c` program.

- Sloane, Andy. "Obfuscated C Donut." _a1k0n.net_, 15 Sep. 2006, [www.a1k0n.net/2006/09/15/obfuscated-c-donut.html](https://www.a1k0n.net/2006/09/15/obfuscated-c-donut.html). The original obfuscated C source code.

- Sloane, Andy. "Optimizing Donut." _a1k0n.net_, 13 Jan. 2021, [www.a1k0n.net/2021/01/13/optimizing-donut.html](https://www.a1k0n.net/2021/01/13/optimizing-donut.html). Follow-up article with performance optimizations.

- Green Code. "I Coded a 3D Spinning Donut." _YouTube_, uploaded by Green Code, [www.youtube.com/watch?v=74FJ8TTMM5E](https://www.youtube.com/watch?v=74FJ8TTMM5E). The video that inspired this project.

- "Torus." _Wikipedia_, Wikimedia Foundation, [en.wikipedia.org/wiki/Torus](https://en.wikipedia.org/wiki/Torus). Mathematical definition and parametric equations for the torus.

- "Rotation Matrix." _Wikipedia_, Wikimedia Foundation, [en.wikipedia.org/wiki/Rotation_matrix](https://en.wikipedia.org/wiki/Rotation_matrix). Standard 3D rotation matrices used for the X-axis and Z-axis rotations.

- "Z-buffering." _Wikipedia_, Wikimedia Foundation, [en.wikipedia.org/wiki/Z-buffering](https://en.wikipedia.org/wiki/Z-buffering). The depth-buffering technique used to handle overlapping surfaces.

- "ANSI Escape Code." _Wikipedia_, Wikimedia Foundation, [en.wikipedia.org/wiki/ANSI_escape_code](https://en.wikipedia.org/wiki/ANSI_escape_code). Reference for the 256-color terminal escape sequences.

- _C++ Reference_. cppreference.com, [en.cppreference.com](https://en.cppreference.com/). Documentation for `<cmath>`, `<chrono>`, `<thread>`, `<cstring>`, and `<iostream>`.
