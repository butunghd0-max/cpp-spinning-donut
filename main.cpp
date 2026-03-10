#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

constexpr int WIDTH = 80;
constexpr int HEIGHT = 24;
constexpr int SIZE = WIDTH * HEIGHT;

constexpr float R1 = 1.0f;
constexpr float R2 = 2.0f;
constexpr float K2 = 5.0f;
constexpr float K1 = (HEIGHT - 4) * K2 / (R1 + R2);

constexpr float THETA_STEP = 0.04f;
constexpr float PHI_STEP = 0.02f;

const char *LUMINANCE = ".,-~:;=!*#$@";

const char *GRADIENT[] = {
    "\x1b[38;5;17m",  //  . dark navy
    "\x1b[38;5;18m",  //  ,
    "\x1b[38;5;19m",  //  -
    "\x1b[38;5;25m",  //  ~ steel blue
    "\x1b[38;5;31m",  //  :
    "\x1b[38;5;37m",  //  ; teal
    "\x1b[38;5;43m",  //  = cyan
    "\x1b[38;5;79m",  //  !
    "\x1b[38;5;115m", //  * light green
    "\x1b[38;5;151m", //  # pale mint
    "\x1b[38;5;188m", //  $ near-white
    "\x1b[38;5;231m", //  @ pure white
};
const char *RESET = "\x1b[0m";

int main() {
  float A = 0, B = 0;
  float zbuf[SIZE];
  char output[SIZE];
  int lum[SIZE];

  std::cout << "\x1b[2J" << "\x1b[?25l"; // clear screen + hide cursor

  auto prev = std::chrono::steady_clock::now();

  while (true) {
    memset(output, 32, SIZE);
    memset(zbuf, 0, SIZE * sizeof(float));
    memset(lum, 0, SIZE * sizeof(int));

    float sinA = sin(A), cosA = cos(A);
    float sinB = sin(B), cosB = cos(B);

    for (float theta = 0; theta < 6.28f; theta += THETA_STEP) {
      float sinT = sin(theta), cosT = cos(theta);

      for (float phi = 0; phi < 6.28f; phi += PHI_STEP) {
        float sinP = sin(phi), cosP = cos(phi);

        float cx = R2 + R1 * cosT;
        float cy = R1 * sinT;

        float x = cx * (cosB * cosP + sinA * sinB * sinP) - cy * cosA * sinB;
        float y = cx * (sinB * cosP - sinA * cosB * sinP) + cy * cosA * cosB;
        float z = cosA * cx * sinP + cy * sinA;

        float invZ = 1.0f / (z + K2);

        int px = (int)(WIDTH / 2 + K1 * invZ * x);
        int py = (int)(HEIGHT / 2 - K1 * invZ * y * 0.5f);
        int idx = px + WIDTH * py;

        int N = (int)(8.0f *
                      ((cosT * cosP * sinB) - cosA * cosT * sinP - sinA * sinT +
                       cosB * (cosA * sinT - cosT * sinA * sinP)));

        if (py > 0 && py < HEIGHT && px > 0 && px < WIDTH && invZ > zbuf[idx]) {
          zbuf[idx] = invZ;
          int L = N > 0 ? N : 0;
          if (L > 11)
            L = 11;
          output[idx] = LUMINANCE[L];
          lum[idx] = L;
        }
      }
    }

    std::string frame;
    frame.reserve(SIZE * 16);
    frame += "\x1b[H";

    for (int j = 0; j < HEIGHT; j++) {
      for (int i = 0; i < WIDTH; i++) {
        int idx = i + WIDTH * j;
        if (output[idx] != ' ') {
          frame += GRADIENT[lum[idx]];
          frame += output[idx];
          frame += RESET;
        } else {
          frame += ' ';
        }
      }
      frame += '\n';
    }

    std::cout << frame << std::flush;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - prev).count();
    prev = now;

    A += 1.2f * dt;
    B += 0.6f * dt;

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  std::cout << "\x1b[?25h"; // show cursor on exit
  return 0;
}
