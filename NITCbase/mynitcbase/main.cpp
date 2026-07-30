#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;

  return FrontendInterface::handleFrontend(argc, argv);
  unsigned char buffer[BLOCK_SIZE];

  //printing hello
  // Disk::readBlock(buffer, 7000);
  // char message[] = "hello";
  // memcpy(buffer + 20, message, 6);
  // Disk::writeBlock(buffer, 7000);
  // unsigned char buffer2[BLOCK_SIZE];
  // char message2[6];
  // Disk::readBlock(buffer2, 7000);
  // memcpy(message2, buffer2 + 20, 6);
  // std::cout << message2 << "\n";

  //modification
  // for (int block = 0; block < 4; block++) {
  //   Disk::readBlock(buffer, block);
  //   for (int i = 0; i < BLOCK_SIZE; i++) 
  //   {
  //     std::cout << (int)buffer[i] << " ";
  //   }
  //   std::cout << std::endl;
  // } 
}