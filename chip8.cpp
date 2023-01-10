#include "chip8.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <vector>

Chip8::Chip8()
    : opcode(0), memory({0}), reg({0}), I(0), PC(0x200), stack({0}), SP(0),
      screen({0}), delay_timer(0), sound_timer(0), key({false}),
      rng(std::random_device{}()), rnd_dist(0, 255) {}

void Chip8::load_rom(const std::string& filename) noexcept {
    reset();
    std::ifstream rom_file(filename, std::ios::in | std::ios::out);
    std::copy((std::istreambuf_iterator<char>(rom_file)),
              std::istreambuf_iterator<char>(), memory.begin() + PC);
}

void Chip8::unknown_opcode() const {
    std::stringstream msg;
    msg << "Unknown opcode: " << std::hex << opcode;
    throw std::runtime_error(msg.str());
}

void Chip8::execute() {
    // read first byte of opcode
    opcode = memory.at(PC);
    // shift by a byte to left
    opcode <<= 8U;
    // read in the second byte of opcode
    opcode |= memory.at(PC + 1);

    // 0x0NNN
    const uint16_t nnn = opcode & 0x0FFF;
    // 0x00NN
    const uint16_t nn = opcode & 0x00FF;
    // 0x0X00
    const uint16_t x = (opcode & 0x0F00) >> 8;
    // 0x00Y0
    const uint16_t y = (opcode & 0x00F0) >> 4;
    // 0x000N
    const uint16_t n = opcode & 0x000F;

    // inspect most significant hex digit of the opcode
    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode & 0x00FF) {
        // CLS
        // clear screen
        case 0x00E0:
            clear_screen();
            PC += 2;
            break;

        // RET
        // return from subroutine
        case 0x00EE:
            --SP;
            PC = stack.at(SP);
            break;

        [[unlikely]] default:
            unknown_opcode();
            break;
        }
        break;

    // JP [NNN]
    // 0x1NNN
    // goto NNN
    case 0x1000:
        PC = nnn;
        break;

    // CALL [NNN]
    // 0x2NNN
    // call subroutine at NNN
    case 0x2000:
        // set return address to address of next opcode
        stack.at(SP) = PC + 2;
        ++SP;
        PC = nnn;
        break;

    // SE VX, NN
    // 0x3XNN
    // skip next instruction if reg[X] == NN
    case 0x3000:
        if (reg.at(x) == nn) {
            PC += 4;
        } else {
            PC += 2;
        }
        break;

    // SNE VX, NN
    // 0x4XNN
    // skip next instruction if reg[X] == NN
    case 0x4000:
        if (reg.at(x) != nn) {
            PC += 4;
        } else {
            PC += 2;
        }
        break;

    // SNE VX, VY
    // 0x5XY0
    // skip next instruction if reg[X] == reg[Y]
    case 0x5000:
        if (reg.at(x) == reg.at(y)) {
            PC += 4;
        } else {
            PC += 2;
        }
        break;

    // LD VX, NN
    // 0x6XNN
    // sets reg[X] to NN
    case 0x6000:
        reg.at(x) = nn;
        PC += 2;
        break;

    // ADD VX, NN
    // 0x7XNN
    // adds NN to reg[X] (doesn't modify carry flag)
    case 0x7000:
        reg.at(x) += nn;
        PC += 2;
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        // LD VX, VY
        // 0x8XY0
        // sets reg[X] to reg[Y]
        case 0x0000:
            reg.at(x) = reg.at(y);
            break;

        // OR VX, VY
        // 0x8XY1
        // sets reg[X] to reg[X] OR reg[Y]
        case 0x0001:
            reg.at(x) |= reg.at(y);
            break;

        // AND VX, VY
        // 0x8XY2
        // sets reg[X] to reg[X] AND reg[Y]
        case 0x0002:
            reg.at(x) &= reg.at(y);
            break;

        // XOR VX, VY
        // 0x8XY3
        // sets reg[X] to reg[X] XOR reg[Y]
        case 0x0003:
            reg.at(x) ^= reg.at(y);
            break;

        // ADD VX, VY
        // 0x8XY4
        // sets reg[X] to reg[X] + reg[Y], reg[0xF] is set to 1 on overflow
        case 0x0004: {
            auto x16 = static_cast<uint16_t>(reg.at(x));
            auto y16 = static_cast<uint16_t>(reg.at(y));
            reg.at(0xF) = x16 + y16 > 0xFF ? 1 : 0;
        }
            reg.at(x) += reg.at(y);
            break;

        // SUB VX, VY
        // 0x8XY5
        // sets reg[X] to reg[X] - reg[Y]
        case 0x0005:
            reg.at(0xF) = reg.at(x) > reg.at(y) ? 1 : 0;
            reg.at(x) -= reg.at(y);
            break;

        // SHR VX
        // 0x8XY6
        // sets reg[X] to reg[X] >> 1, reg[0xF] is set to shifted bit
        case 0x0006:
            reg.at(0xF) = reg.at(x) & 1U;
            reg.at(x) >>= 1U;
            break;

        // SUBN VX, VY
        // 0x8XY7
        // sets reg[X] to reg[Y] - reg[X], reg[0xF] is set to 1 on overflow
        case 0x0007:
            reg.at(0xF) = reg.at(y) > reg.at(x) ? 1 : 0;
            reg.at(x) = reg.at(y) - reg.at(x);
            break;

        // SHL VX
        // 0x8XYE
        // sets reg[X] to reg[X] << 1, reg[0xF] is set to shifted bit
        case 0x000E:
            reg.at(0xF) = (reg.at(x) >> 7U) & 1U;
            reg.at(x) <<= 1U;
            break;

        [[unlikely]] default:
            unknown_opcode();
            break;
        }
        PC += 2;
        break;

    // SNE, NN
    // 0x9XY0
    // skips next instruction if reg[X] != reg[Y]
    case 0x9000:
        if (reg.at(x) != reg.at(y)) {
            PC += 4;
        } else {
            PC += 2;
        }
        break;

    // LD I, NNN
    // 0xANNN
    // sets register I to NNN
    case 0xA000:
        I = nnn;
        PC += 2;
        break;

    // JP V0, [NNN]
    // 0xBNNN
    // jumps to location NNN + reg[0]
    case 0xB000:
        PC = nnn + reg.at(0);
        break;

    // RND VX, NN
    // 0xCXNN
    // sets reg[X] to a random number in range 0-255 ANDed with NN
    case 0xC000:
        reg.at(x) = rnd_dist(rng) & nn;
        PC += 2;
        break;

    // DRW VX, VX, VN
    // 0xDXYN
    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a
    // height of N pixels. Each row of 8 pixels is read as bit-coded starting
    // from memory location I; I value does not change after the execution of
    // this instruction. As described above, VF is set to 1 if any screen pixels
    // are flipped from set to unset when the sprite is drawn, and to 0 if that
    // does not happen.
    case 0xD000:
        reg.at(0xF) = 0;
        for (uint16_t row = 0; row < n; ++row) {
            for (uint16_t col = 0; col < 8; ++col) {
                uint16_t draw_x = (col + reg.at(x)) % Chip8::WIDTH;
                uint16_t draw_y = (row + reg.at(y)) % Chip8::HEIGHT;

                // if (draw_x > WIDTH - 1) {
                //     draw_x %= WIDTH;
                // };
                // if (draw_y > HEIGHT - 1) {
                //     draw_y %= HEIGHT;
                // };

                bool bit = memory.at(I + row) & (0x80 >> col);
                bool previous_bit = screen.at(draw_y)[draw_x];

                if (bit) {
                    screen.at(draw_y).flip(draw_x);
                }

                // a pixel was erased
                if (previous_bit and not screen.at(draw_y)[draw_x]) {
                    reg.at(0xF) = 1;
                }
            }
        }
        draw_pending = true;
        PC += 2;
        break;

    case 0xE000:
        switch (opcode & 0x00FF) {
        // SKP VX
        // 0xEX9E
        // skip next instruction if key reg[x] was pressed
        case 0x009E:
            if (key.at(reg.at(x))) {
                PC += 2;
            }
            PC += 2;
            break;

        // SKNP VX
        // 0xEX9E
        // skip next instruction if key reg[x] was not pressed
        case 0x00A1:
            if (not key.at(reg.at(x))) {
                PC += 2;
            }
            PC += 2;
            break;

        [[unlikely]] default:
            unknown_opcode();
            break;
        }
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        // LD VX, DT
        // 0xFX07
        // value of delay timer is set to reg[x]
        case 0x0007:
            reg.at(x) = delay_timer;
            break;

        // LD VX, DT
        // 0xFX0A
        // wait for key press, store the key index in reg[x]
        // all execution stops until a key is pressed
        case 0x000A:
            wait_for_key = true;
            for (unsigned i = 0; i < key.size(); ++i) {
                if (key.at(i)) {
                    reg.at(x) = i;
                    wait_for_key = false;
                }
            }
            if (wait_for_key) {
                return;
            }
            break;

        // LD DT, VX
        // 0xFX15
        // sets delay timer to reg[x]
        case 0x0015:
            delay_timer = reg.at(x);
            break;

        // LD ST, VX
        // 0xFX18
        // sets sound timer to reg[x]
        case 0x0018:
            sound_timer = reg.at(x);
            break;

        // ADD I, VX
        // 0xFX1E
        // sets I to I + reg[x]
        case 0x001E:
            if (I + reg.at(x) > 0xFFF) {
                reg.at(0xF) = 1;
            } else {
                reg.at(0xF) = 0;
            }
            I += reg.at(x);
            break;

        // LD F, VX
        // 0xFX29
        // sets I to address of sprite for digit reg[x]
        case 0x0029:
            // (each font is 5 bytes)
            I = 5 * reg.at(x);
            break;

        // LD B, VX
        // 0xFX33
        // The interpreter takes the decimal value of Vx,
        // and places the hundreds digit in memory at location in I,
        // the tens digit at location I+1, and the ones digit at location I+2.
        case 0x0033:
            memory.at(I) = reg.at(x) % 1000 / 100;
            memory.at(I + 1) = reg.at(x) % 100 / 10;
            memory.at(I + 2) = reg.at(x) % 10;
            break;

        // LD [I], VX
        // 0xFX55
        // store registers in range reg[0] - reg[X] starting from location
        // pointed by I
        case 0x0055:
            for (auto i = 0; i <= x; ++i) {
                memory.at(I + i) = reg.at(i);
            }
            // On the original interpreter, when the
            // operation is done, I = I + X + 1.
            // I += x + 1;
            break;

        // LD VX, [I]
        // 0xFX65
        // read registers in range reg[0] - reg[X] starting from location
        // pointed by I
        case 0x0065:
            for (auto i = 0; i <= x; ++i) {
                reg.at(i) = memory.at(I + i);
            }
            // On the original interpreter, when the
            // operation is done, I = I + X + 1.
            // I += x + 1;
            break;

        [[unlikely]] default:
            unknown_opcode();
            break;
        }

        PC += 2;
        break;

    [[unlikely]] default:
        unknown_opcode();
        break;
    }

    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto current_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // update timers if 1/60th of a second has passed since last update
    // 16.666... rounded to 17
    if (current_time_ms - last_timer_update_ms >= 17) {
        last_timer_update_ms = current_time_ms;

        if (delay_timer > 0) {
            --delay_timer;
        }

        if (sound_timer > 0) {
            // TODO: implement sound playback
            --sound_timer;
        }
    }
}
