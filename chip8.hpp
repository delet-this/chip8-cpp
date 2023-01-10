#include <array>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>

#ifndef CHIP8
#define CHIP8

class Chip8 {
  public:
    Chip8();
    static constexpr int WIDTH = 64;
    static constexpr int HEIGHT = 32;

    constexpr void reset() noexcept {
      // program counter starts at 0x200 by spec
      PC = 0x200;
      // zero I register
      I = 0;
      // zero stack pointer
      SP = 0;
      // zero current opcode
      opcode = 0;

      // constexpr std::array::fill requires C++20!

      // zero stack
      std::fill(stack.begin(), stack.end(), 0);
      // zero registers
      std::fill(reg.begin(), reg.end(), 0);
      // zero memory
      std::fill(memory.begin(), memory.end(), 0);
      // load font to memory
      std::copy(font.begin(), font.end(), memory.begin());
      // zero keys
      std::fill(key.begin(), key.end(), 0);
      // zero screen
      clear_screen();
      // zero timers
      delay_timer = 0;
      sound_timer = 0;

      last_timer_update_ms = 0;

      draw_pending = false;
    }

    [[noreturn]] void unknown_opcode() const;
    void execute();

    constexpr void clear_screen() noexcept {
      for (auto& row : screen) {
        row.reset();
      }
      draw_pending = true;
    }

    void load_rom(const std::string& filename) noexcept;
    [[nodiscard]] constexpr bool is_draw_pending() const noexcept { return draw_pending; };

    constexpr void clear_draw_pending() noexcept { draw_pending = false; };

    [[nodiscard]] constexpr const std::array<std::bitset<Chip8::WIDTH>, Chip8::HEIGHT>&
    get_screen() const noexcept {
        return screen;
    }

    constexpr void key_down(unsigned i) { key.at(i) = true; };
    constexpr void key_up(unsigned i) { key.at(i) = false; };

  private:
    static constexpr std::array<uint8_t, 80> font = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    uint16_t opcode;
    std::array<uint8_t, 4096> memory;
    std::array<uint8_t, 16> reg;
    uint16_t I;
    uint16_t PC;
    std::array<uint16_t, 16> stack;
    uint8_t SP;
    std::array<std::bitset<WIDTH>, HEIGHT> screen;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool wait_for_key = false;
    std::array<bool, 16> key;
    std::mt19937 rng;
    std::uniform_int_distribution<std::mt19937::result_type> rnd_dist;
    bool draw_pending = false;
    std::chrono::duration<long, std::ratio<1, 1000>>::rep last_timer_update_ms =
        0;
};

#endif // !CHIP8
