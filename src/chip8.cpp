#include <fstream>
#include <iosfwd>

#include "../headers/chip8.h"

const unsigned int START_ADDRESS = 0x200;

Chip8::Chip8() { this->pc = START_ADDRESS; }

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
