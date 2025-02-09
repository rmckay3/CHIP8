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
/// SUB Vx, Vy: Set Vx = Vx - vy, set VF = NOT borrow (Vx > Vy)
///
void Chip8::OP_8xy5() {
  u_int8_t Vx = (this->opcode & 0x0F00u) >> 8u;
  u_int8_t Vy = (this->opcode & 0x00F0u) >> 4u;

  this->registers[0xF] = Vx > Vy;
  this->registers[Vx] -= this->registers[Vy];
}
