/*

  # LMSPaser

  Parser for ASCII data sent from a LMS1XX device, based on 
  this [pdf](https://www.mysick.com/saqqara/im0045927.pdf). 
  As I only have remote access to a LMS device, I cannot test this 
  class thoroughly atm;  Parsing is not yet optimized; 
  
  @author Diederick Huijbers <diederick[some_common_symbol]apollomedia.nl>

 */

#ifndef ROXLU_LMS_PARSER_H
#define ROXLU_LMS_PARSER_H

#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdint.h>
#include <inttypes.h>

/* parse states */
#define LMS_STATE_NONE 0
#define LMS_STATE_STX 1
#define LMS_STATE_COMMAND 2
#define LMS_STATE_COMMAND_PARAM 3
#define LMS_STATE_ETX 4

/* master commands */
#define LSM_COMMAND_SAN 0x0073414E
#define LSM_COMMAND_SRA 0x00735241
#define LSM_COMMAND_SEA 0x00734541
#define LSM_COMMAND_SWA 0x00735741
#define LSM_COMMAND_SSN 0x0073534E

class LMSParser {
 public:
  LMSParser();

  /* PARSE! */
  bool parse();                                                                                                    /* returns true when we parsed one or multiple messages */

  /* READING */
  std::string readHexNumberAsString(size_t maxChars = 8, uint8_t stop = 0x03);                                     /* will read the next bytes as hex, we stop when we see a space or a ETX character (0x03); we flush up to the space, but not the space itself */
  uint8_t readHexNumberAsU8();                                                                                     /* read the next byte as hex and convert it to uint8_t */
  uint16_t readHexNumberAsU16(uint8_t stop = 0x03);                                                                /* read the next bytes as hex and convert it to uint16_t */
  uint32_t readHexNumberAsU32(uint8_t stop = 0x03);                                                                /* read the next bytes as hex and convert it to uint32_t */
  int32_t readHexNumberAsI32(uint8_t stop = 0x03);                                                                 /* read the next bytes as hex and convert it to int32_t */
  int16_t readHexNumberAsI16(uint8_t stop = 0x03);                                                                 /* read the next bytes as hex and convert it to int16_t */
  float readHexNumberAsFloat(uint8_t stop = 0x03);                                                                 /* read the next bytes as hex and convert it to  float */
  void flush(size_t nbytes);                                                                                       /* flush X-bytes, reducing the buffer size */

  /* DEBUG */
  void println();                                                                                                  /* just prints the current X bytes */
  std::string stlmsStatusCodeToString(uint16_t code);                                                              /* helper to convert the stlm status string */
  
 public:
  std::vector<uint16_t> chan_data;
  std::vector<uint8_t> data;
  int state;
  uint32_t command;
  std::string command_name;
  size_t bytes_flushed;
};

#endif
