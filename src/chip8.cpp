#include <chrono>
#include <cstring>
#include <fstream>
#include <iosfwd>
#include <random>
#include <sys/types.h>

#include "../headers/chip8.h"

const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSENT_START_ADDRESS = 0x50;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;

uint8_t fontset[FONTSET_SIZE] = {
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

Chip8::Chip8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
  this->pc = START_ADDRESS;
  this->randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

  for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
    this->memory[FONTSENT_START_ADDRESS + i] = fontset[i];
  }
}

void Chip8::LoadROM(char const *filename) {
  // Open file as stream of binary and move pointer to end
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (file.is_open()) {
    // Get size of file and allocate buffer
    std::streampos size = file.tellg();
    char *buffer = new char[size];

    // Move pointer to beginning and read into buffer
    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    file.close();

    // Load ROM into Chip8 Memory
    for (long i = 0; i < size; ++i) {
      this->memory[START_ADDRESS + i] = buffer[i];
    }

    delete[] buffer;
  }
}

///
/// CLS: Clear Display
///
void Chip8::OP_00E0() { memset(this->video, 0, sizeof(this->video)); }

///
/// RET: Return from Subroutine
///
void Chip8::OP_00EE() {
  --this->sp;
  this->pc = this->stack[this->sp];
}

///
/// JP addr: Jump to location nnn. PC set to nnn
///
void Chip8::OP_1nnn() {
  u_int16_t address = this->opcode & 0x0FFFu;
  this->pc = address;
}

///
/// CALL addr: Call Subroutine at nnn.
///
void Chip8::OP_2nnn() {
  u_int16_t address = this->opcode & 0x0FFFu;

  this->stack[this->sp] = this->pc;
  ++this->sp;
  this->pc = address;
}

///
/// SE Vx, byte: Skip next instruction if register Vx == kk
///
void Chip8::OP_3xkk() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t byte = this->opcode & 0x00FFu;

  if (this->registers[Vx] == byte) {
    this->pc += 2;
  }
}

///
/// SNE Vx, byte: Skip next instruction if register Vx != kk
void Chip8::OP_4xkk() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t byte = this->opcode & 0x00FFu;

  if (this->registers[Vx] != byte) {
    this->pc += 2;
  }
}

///
/// SE Vx, Vy: skip next instruciton if register Vx == register Vy
///
void Chip8::OP_5xy0() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  if (this->registers[Vx] == this->registers[Vy]) {
    this->pc += 2;
  }
}

///
/// LD Vx, byte: Set Vx == kk;
///
void Chip8::OP_6xkk() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t byte = this->opcode & 0x00FFu;

  this->registers[Vx] = byte;
}

///
/// ADD Vx, Byte: Add value to Vx register
///
void Chip8::OP_7xkk() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t byte = this->opcode & 0x00FFu;

  this->registers[Vx] += byte;
}

///
/// LD Vx, Vy: Set register Vx to Vy
///
void Chip8::OP_8xy0() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[Vx] = this->registers[Vy];
}

///
/// OR Vx, Vy: Set Vx = Vx | Vy
///
void Chip8::OP_8xy1() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[Vx] |= this->registers[Vy];
}

///
/// AND Vx, Vy: Set Vx = Vx & Vy
///
void Chip8::OP_8xy2() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[Vx] &= this->registers[Vy];
}

///
/// XOR Vx, Vy: Set Vx = Vx ^ Vy
///
void Chip8::OP_8xy3() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[Vx] ^= this->registers[Vy];
}

///
/// ADD Vx, Vy: Set Vx = Vx + Vy, set VF = carry
///
void Chip8::OP_8xy4() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  u_int16_t sum = Vx + Vy;

  this->registers[0xF] = sum > 255u;
  this->registers[Vx] = sum & 0xFFu;
}

///
/// SUB Vx, Vy: Set Vx = Vx - Vy, set VF = NOT borrow (Vx > Vy)
///
void Chip8::OP_8xy5() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[0xF] = this->registers[Vx] > this->registers[Vy];
  this->registers[Vx] -= this->registers[Vy];
}

///
/// SHR Vx: If the least significant digit of Vx is 1, Set VF to 1. Otherwise VF = 0.
/// Divide Vx by 2
///
void Chip8::OP_8xy6() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->registers[0xF] = this->registers[Vx] & 01u;
  this->registers[Vx] >>= 1; // Shift right 1 == Divide by 2;
}

///
/// SUB Vy, Vx: Set Vx = Vy - Vx, set VF = NOT borrow (Vy > Vx)
///
void Chip8::OP_8xy7() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[0xF] = this->registers[Vy] > this->registers[Vx];
  this->registers[Vx] = this->registers[Vy] - this->registers[Vx];
}

///
/// SHL Vx: Save Most significant bit in VF. Multiple by 2
///
void Chip8::OP_8xyE() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->registers[0xF] = (this->registers[Vx] & 0x80u) >> 7u;
  this->registers[Vx] <<= 1;
}

///
/// SNE Vx, Vy: Skip next instruction if Vx != Vy
///
void Chip8::OP_9xy0() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  if (this->registers[Vx] != this->registers[Vy]) { this->pc += 2; }
}


///
/// Set I = nnn
///
void Chip8::OP_Annn() {
  u_int16_t address = this->opcode & 0x0FFFu;

  this->index = address;
}

///
/// JMP V0, addr: Jump to location + V0
///
void Chip8::OP_Bnnn() {
  u_int16_t address = this->opcode & 0x0FFFu;

  this->pc = this->registers[0] + address;
}

///
/// RND Vx, byte: Set Vx to Rand Byte AND kk
///
void Chip8::OP_Cxkk() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t byte = this->opcode & 0x00FFu;

  this->registers[Vx] = this->randGen(this->randByte) & byte;
}

///
/// DRW Vx, Vy, nibble: Display n-sprite at memory location I at (Vx, Vy), set VF = collision
///
void Chip8::OP_Dxyn() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;
  u_int8_t height = this->opcode & 0x000Fu;

  u_int8_t x = this->registers[Vx] % VIDEO_WIDTH;
  u_int8_t y = this->registers[Vy] % VIDEO_HEIGHT;

  this->registers[0xF] = 0;

  for (unsigned int row = 0; row < height; ++row) {
    uint8_t spriteByte = this->memory[this->index + row];

    for (unsigned int col = 0; col < 8; ++col) {
      uint8_t spritePixel = spriteByte & (0x80u >> col);
      uint32_t* screenPixel = &this->video[(y + row) * VIDEO_WIDTH + (x + col)];

      if (spritePixel) {

        if (*screenPixel == 0xFFFFFFFF) { this->registers[0xF] = 1; }

        *screenPixel ^= 0xFFFFFFFF;
      }
    }
  }
}

///
/// SKP Vx: Skip instruction if Key with value of Vx is pressed
///
void Chip8::OP_Ex9E() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  uint8_t key = this->registers[Vx];

  if (this->keypad[key]) { this->pc += 2; }
}

///
/// SKNP Vx: Skip instruction if Key with value of Vx is not pressed
///
void Chip8::OP_ExA1() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  uint8_t key = this->registers[Vx];

  if (!this->keypad[key]) { this->pc += 2; }
}

///
/// LD Vx, DT: Set Vx = Delay Timer
///
void Chip8::OP_Fx07() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->registers[Vx] = this->delayTimer;
}

///
/// LD Vx, K: Wait for Key Press, store key value in Vx
///
void Chip8::OP_Fx0A() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  uint8_t key = 16

  for (unsigned int keyPress = 0; keyPress < key; ++keyPress) {
    if (this->keypad[keyPress]) {
      key = keyPress;
      this->registers[Vx] = key;
    }
  }

  if (key > 15) { this->pc -= 2; } // Repeat instruction until key is pressed
}

///
/// LD DT, Vx: Set Delay timer to value in Vx
///
void Chip8::OP_Fx15() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->delayTimer = this->registers[Vx];
}

///
/// LD ST, Vx: Set Sound Timer to value in Vx
///
void Chip8::OP_Fx18() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->soundTimer = this->registers[Vx];
}

///
/// ADD I, Vx: Set I = I + Vx
///
void Chip8::OP_Fx1E() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  this->index += this->registers[Vx];
}

///
/// LD F, Vx: Set I = location of sprite for Digit Vx
///
void Chip8::OP_Fx29() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  uint8_t digit = this->registers[Vx];

  this->index = FONTSENT_START_ADDRESS + (5 * digit);
}

///
/// LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2
/// I gets hundreds place, I+1 gets tens place, I+2 gets ones place
///
void Chip8::OP_Fx33() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  uint8_t value = this->registers[Vx];

  // Hundreds Place
  this->memory[this->index + 2] = value % 10;
  value /= 10;

  // Tens Place
  this->memory[this->index + 1] = value % 10
  value /= 10;

  // Ones Place
  this->memory[this->index] = value % 10;
}

///
/// LD [I], Vx: Store Registers V0 through Vx in memory starting at I
///
void Chip8::OP_Fx55() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  for (uint8_t i = 0; i < Vx; ++i) {
    this->memory[this->index + i] = this->registers[i];
  }
}

///
/// LD Vx, [I]: Read Registers V0 through Vx from memory starting at I
///
void Chip8::OP_Fx65() {
  uint8_t Vx = (this->opcode & 0x0F00u) >> 8u;

  for (uint8_t i = 0; i < Vx; ++i) {
    this->registers[i] = this->memory[this->index + i];
  }
}