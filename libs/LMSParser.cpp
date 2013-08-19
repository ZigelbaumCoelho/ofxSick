#include "LMSParser.h"

#define LMS_DEBUG 0

// --------------------------------------------------------
LMSParser::LMSParser()
  :state(LMS_STATE_NONE)
  ,command(0)
  ,bytes_flushed(0)
{

}

bool LMSParser::parse() {
  while(data.size()) {
    switch(state) {

      case LMS_STATE_NONE: {
        bool must_continue = false;
        for(size_t i = 0; i < data.size(); ++i) {
          if(data[i] == 0x02) {
            state = LMS_STATE_STX;
#if LMS_DEBUG
            printf("+ found STX at %ld\n", i);
            println();
#endif
            must_continue = true;
            break;
          }
        }

        if(must_continue) {
          continue;
        }
        else {
          return false;
        }

      };

      case LMS_STATE_STX: {
        // too small
        if(data.size() < 4) {
          printf("- not enough data in buffer for LSM_STATE_STX\n");
          return false;
        }

        char* ptr = (char*)&command;
        ptr[2] = data[1];
        ptr[1] = data[2];
        ptr[0] = data[3];

#if 0
        unsigned char* pp = (unsigned char*) ptr;
        printf("%02X %02X %02X\n", pp[2], pp[1], pp[0]);
        printf(">>>>>>>>>>>> %ld <<<<<<<<<<<<\n", bytes_flushed);
#endif     

        switch(command) {
          case LSM_COMMAND_SSN:
          case LSM_COMMAND_SEA:
          case LSM_COMMAND_SAN:
          case LSM_COMMAND_SRA:
          case LSM_COMMAND_SWA: {
            flush(4);
            state = LMS_STATE_COMMAND;
            break;
          }
          default: {
            // unknown command .. oops
            unsigned char* p = (unsigned char*)ptr;
            printf("--\n");
            printf("Unhandled command! %02X%02X%02X, we shouldn't arrive here. The unhandled command is:\n", ptr[0], ptr[1], ptr[2]);
            println();
            printf("--\n");
            return false;
          }
        };
        continue; // continue with parsing
      };

      case LMS_STATE_COMMAND: {
        bool should_continue = false;

        switch(command) {
          case LSM_COMMAND_SSN:
          case LSM_COMMAND_SEA:
          case LSM_COMMAND_SWA:
          case LSM_COMMAND_SRA:
          case LSM_COMMAND_SAN: {
            for(size_t i = 1; i < data.size(); ++i) {
        
              if(data[i] == ' ' || data[i] == 0x03) {
                should_continue = true;
                state = LMS_STATE_COMMAND_PARAM;
                int space = (data[i] == 0x03) ? 0 : 1;
                flush(i + space);
                break;
              }

              command_name.push_back(data[i]);
            }

            break;
          }
          default: {
            printf("Unhandled command type.\n");
            return false;
          }
        };

        if(should_continue) {
          continue;
        }
        else {
          printf("- not enough data in buffer for LSM_STATE_COMMAND\n");
          command_name.clear();
          return false;
        }
      };

      case LMS_STATE_COMMAND_PARAM: {
        if(data.size() == 0) {
#if LMS_DEBUG
          printf("- not enough data in buffer for LSM_STATE_COMMAND_PARAM\n");
#endif
          return false;
        }

        if(command_name == "SetAccessMode") {
          printf("+ %s\n", command_name.c_str());
          if(data[0] == 0x31) {
            printf("  + SetAccessMode: OK\n");
          }
          else if (data[0] == 0x30) {
            printf("  - SetAccessMode: ERROR.\n");
          }

          flush(1);
          state = LMS_STATE_ETX;
        }
        else if(command_name == "LMCstopmeas") {
          printf("+ %s\n", command_name.c_str());
          if(data[0] == 0x031) {
            printf("  + LMCstopmeas: OK\n");
          }
          else {
            printf("  - LMCStopmeas: NOT ALLOWED\n");
          }

          flush(1);
          state = LMS_STATE_ETX;
        }
        else if(command_name == "mLMPsetscancfg") {
          // the size is variable so we need to read up to the next ETX
          bool has_enough = false;
          for(size_t i = 0; i < data.size(); ++i) {
            if(data[i] == 0x03) {
              has_enough = true;
              break;
            }
          }
          if(!has_enough) {
            printf("- not enough data in buffer for LSM_STATE_COMMAND_PARAM - mLMPsetscancfg\n");
            return false;
          }

          printf("+ %s\n", command_name.c_str());

          switch(data[0]) {
            case 0x30: {  printf("  + status code: OK\n");                            break; } 
            case 0x31: {  printf("  - status code: Frequency Error\n");               break; }
            case 0x32: {  printf("  - status code: Resolution Error\n");              break; }
            case 0x33: {  printf("  - status code: Resn. and Scn. Error\n");          break; }
            case 0x34: {  printf("  - status code: Scan area Error.\n");              break; }
            case 0x35: {  printf("  - status code: Other Error.\n");                  break; }
            default:   {  printf("  - status code: UNKNOWN ERROR! - INVALID DATA\n"); break; } 
            break;
          }

          flush(2);
          {
            uint32_t scan_freq = 0;
            int16_t reserved = 0;
            uint32_t angle_res = 0;
            int32_t start_angle = 0;
            int32_t stop_angle = 0;

            scan_freq = readHexNumberAsU32();
            flush(1); // space
            reserved = readHexNumberAsI16();
            flush(1); // space
            angle_res = readHexNumberAsU32();
            flush(1); // space
            start_angle = readHexNumberAsI32();
            flush(1); // space
            stop_angle = readHexNumberAsI32();

            printf("  + scan_freq: %d \n", scan_freq);
            printf("  + angle_res: %d \n", angle_res);
            printf("  + start_angle: %d\n", start_angle);
            printf("  + stop_angle: %d\n", stop_angle);

            state = LMS_STATE_ETX;
          }
        }
        else if(command_name == "LMPscancfg") {
          // variable data - check for end character
          bool has_enough = false;
          for(size_t i = 0; i < data.size(); ++i) {
            if(data[i] == 0x03) {
              has_enough = true;
              break;
            }
          }
          if(!has_enough) {
#if LMS_DEBUG
            printf("- not enough data in buffer for LSM_STATE_COMMAND_PARAM - LMPscancfg\n");
#endif
            return false;
          }

          uint32_t scan_freq = 0;
          int16_t reserved = 0;
          uint32_t angle_res = 0;
          int32_t start_angle = 0;
          int32_t stop_angle = 0;

          scan_freq = readHexNumberAsU32();
          flush(1);
          reserved = readHexNumberAsI16();
          flush(1);
          angle_res = readHexNumberAsU32();
          flush(1); 
          start_angle = readHexNumberAsI32();
          flush(1);
          stop_angle = readHexNumberAsI32();

          printf("+ %s\n", command_name.c_str());
          printf("  + scan_freq: %d \n", scan_freq);
          printf("  + angle_res: %d \n", angle_res);
          printf("  + start_angle: %d\n", start_angle);
          printf("  + stop_angle: %d\n", stop_angle);
          
          state = LMS_STATE_ETX;
          
        }
        else if(command_name == "LMDscandatacfg") {
          printf("+ %s\n", command_name.c_str());
          state = LMS_STATE_ETX;
        }
        else if(command_name == "LMCstartmeas") {
          printf("+ %s\n", command_name.c_str());
          if(data[0] == 0x30) {
            printf("  - statuscode: OK\n");
          }
          else if(data[1] == 0x31) {
            printf("  - statuscode: NOT ALLOWED\n");
          }
          else {
            printf("  - statuscode: UNHANDLED STATUS CODE\n");
          }
          flush(1);
          state = LMS_STATE_ETX;
        }
        else if(command_name == "STlms") {
          // variable data - check for end character
          bool has_enough = false;
          for(size_t i = 0; i < data.size(); ++i) {
            if(data[i] == 0x03) {
              has_enough = true;
              break;
            }
          }
          if(!has_enough) {
            printf("- not enough data in buffer for LSM_STATE_COMMAND_PARAM - STLms\n");
            return false;
          }

          uint16_t status_code = 0;
          uint8_t op_temp = 0;
          uint16_t unknown = 0;
          uint16_t time_h = 0;
          uint16_t time_m = 0;
          uint16_t time_s = 0;
          uint16_t date_d = 0;
          uint16_t date_m = 0;
          uint32_t date_y = 0;
          uint16_t led1 = 0;
          uint16_t led2 = 0;
          uint16_t led3 = 0;

          status_code = readHexNumberAsU16();
          flush(1); // space

          op_temp = readHexNumberAsU8();
          flush(1); // space

          unknown = readHexNumberAsU16();
          flush(1); // space

          time_h = readHexNumberAsU16(0x3a);
          flush(1); // space
          
          time_m = readHexNumberAsU16(0x3A);
          flush(1); // space
          
          time_s = readHexNumberAsU16(0x3A);          
          flush(1); // space

          unknown = readHexNumberAsU16();
          flush(1); // space

          date_d = readHexNumberAsU16(0x2E);
          flush(1); // space

          date_m = readHexNumberAsU16(0x2E);
          flush(1); // space
          
          date_y = readHexNumberAsU32();
          flush(1); // space
          
          led1 = readHexNumberAsU16();
          flush(1); // space

          led2 = readHexNumberAsU16();
          flush(1); // space 

          led3 = readHexNumberAsU16();
#if LMS_DEBUG
          printf("+ %s\n", command_name.c_str());
          printf("  + status_code: %s\n", stlmsStatusCodeToString(status_code).c_str());
          printf("  + op.temp. range: %d\n", op_temp);
          printf("  + time: %d:%d:%d\n", time_h, time_m, time_s);
          printf("  + date: %02d.%02d.%04d\n", date_d, date_m, 2000 +date_y);
          printf("  + led1: %d\n", led1);
          printf("  + led2: %d\n", led2);
          printf("  + led3: %d\n", led3);
#endif
          state = LMS_STATE_ETX;

        } // STLms
        else if(command_name == "LMDscandata") {

          if(command == LSM_COMMAND_SEA) {

            printf("+ %s\n", command_name.c_str());
            if(data[0] == 0x30) {
              printf("  + stop\n");
            }
            else if(data[0] == 0x31) {
              printf("  + start\n");
            }
            else {
              printf("  - UNKNOWN STATE\n");
              return false;
            }
            flush(1);
            state = LMS_STATE_ETX;
          } // LSM_COMMAND_SEA
          else if(command == LSM_COMMAND_SSN) {

            // variable data - check for end character
            bool has_enough = false;
            for(size_t i = 0; i < data.size(); ++i) {
              if(data[i] == 0x03) {
                has_enough = true;
                break;
              }
            }
            if(!has_enough) {
#if LMS_DEBUG
              printf("- not enough data in buffer for LSM_STATE_COMMAND_PARAM - LMDscandata\n");
#endif
              return false;
            }

            // ----------------------------------------------------------------------------------------
            uint16_t version = 0;
            uint16_t device_number = 0;
            uint32_t serial_number = 0;
            uint8_t device_status = 0;
            uint16_t tel_counter = 0;
            uint16_t scan_counter;
            uint32_t time_since_startup = 0;
            uint32_t time_of_transmission = 0;
            uint8_t dig_input_status = 0;
            uint8_t dig_output_status = 0;
            uint16_t reserved = 0;
            uint32_t scan_freq = 0;
            uint32_t measurement_freq;
            uint16_t amount_of_encoder = 0;
            uint16_t encoder_position = 0;
            uint16_t encoder_speed = 0;
            uint16_t amount_of_16bit_channels = 0;
            std::string channel_content_name;
            float scale_factor = 0.0f;
            float scale_factor_offset = 0.0f;
            uint32_t start_angle = 0;
            uint16_t steps = 0;
            uint16_t amount_of_data = 0;
            uint16_t amount_of_8bit_channels = 0;
            uint16_t position = 0;
            uint16_t device_name_flag = 0;
            uint8_t device_name_len = 0;
            std::string device_name;
            uint16_t comment = 0;
            uint16_t time_info_flag = 0;
            uint16_t date_info_y = 0; // year
            uint8_t date_info_m = 0; // month
            uint8_t date_info_d = 0; // day 
            uint8_t time_info_h = 0; // hour
            uint8_t time_info_m = 0; // minute
            uint8_t time_info_s = 0; // seconds
            uint32_t time_info_us = 0; // usecs
            uint16_t event_info_flag = 0;
            
            version = readHexNumberAsU16();
            flush(1); // space
            device_number = readHexNumberAsU16();
            flush(1); // space
            serial_number = readHexNumberAsU32();
            flush(1); // space
            flush(1); // first status byte (always zero in doc)
            device_status = readHexNumberAsU8();
            flush(1); // space
            tel_counter = readHexNumberAsU16();
            flush(1); // space
            scan_counter = readHexNumberAsU16();
            flush(1); // space
            time_since_startup = readHexNumberAsU32();
            flush(1);
            time_of_transmission = readHexNumberAsU32();

            flush(1); // space

            flush(5); // ---------------------------------- WHAT ARE THESE BYTES ? 
            flush(1); // space

            // input
            dig_input_status = readHexNumberAsU8();
            flush(1); // space
            dig_input_status = readHexNumberAsU8();
            flush(1); // space

            // output
            dig_output_status = readHexNumberAsU8();
            flush(1); //space
            dig_output_status = readHexNumberAsU8();
            flush(1); // space

            // reserved
            flush(2);
            
            scan_freq = readHexNumberAsU32();
            flush(1); // space
            
            measurement_freq = readHexNumberAsU32();
            flush(1); // space

            amount_of_encoder = readHexNumberAsU16();
            flush(1); // space

            if(amount_of_encoder != 0) {
              encoder_position = readHexNumberAsU16();
              flush(1); // space

              encoder_speed = readHexNumberAsU16();
              flush(1); // space
            }


            amount_of_16bit_channels = readHexNumberAsU16();
            flush(1); // space
            
            channel_content_name.assign((char*)&data[0], ((char*)&data[0])+5);
            flush(5); // name
            flush(1); // space

            scale_factor = readHexNumberAsFloat(); // ------------------------ @todo The documentation says that the values need to be scaled ... not implemented that
            flush(1); // space

            scale_factor_offset = readHexNumberAsFloat();
            flush(1); // space

            start_angle = readHexNumberAsU32();
            flush(1); // space

            steps = readHexNumberAsU32();
            flush(1); // space

            amount_of_data = readHexNumberAsU16();
            flush(1); // space

            uint16_t data_n = 0;
            if(channel_content_name == " 3F80") {
              chan_data.clear();
              for(size_t i = 0; i < amount_of_data; ++i) {
                data_n = readHexNumberAsU16();
                chan_data.push_back(data_n);
                flush(1); // space
              }
            }
            else {
              for(size_t i = 0; i < amount_of_data; ++i) {
                data_n = readHexNumberAsU16();
                flush(1); // space
              }
            }
            
            amount_of_8bit_channels = readHexNumberAsU16();
            if(amount_of_8bit_channels > 0) {
              printf("ERROR: The amount of 8 bit channels is not 0. We need to implement this.\n");
              ::exit(EXIT_FAILURE);
            }
            flush(1); // space

            position = readHexNumberAsU16();
            flush(1); // space
            
            device_name_flag = readHexNumberAsU16();
            if(device_name_flag == 0x01) {
              printf("ERROR: There is a device name in the data. We need to implement parsing this.\n");
              ::exit(EXIT_FAILURE);
            }
            flush(1); // space

            comment = readHexNumberAsU16();
            flush(1); // space

            time_info_flag = readHexNumberAsU16();
            if(time_info_flag != 0) {
              printf("ERROR: The time info flag is not 0. We did not yet implement parsing of the time values.\n");
              ::exit(EXIT_FAILURE);
            }
            flush(1); // space

            event_info_flag = readHexNumberAsU16();
            if(event_info_flag != 0) {
              printf("ERROR: The event info flag is not 0. We did not yet implement parsing of the event info.\n");
              ::exit(EXIT_FAILURE);
            }
#if LMS_DEBUG
            printf("  + version: %d\n", version);
            printf("  + device number: %d\n", device_number);
            printf("  + serial number: %d\n", serial_number);
            printf("  + device status: %d\n", device_status);
            printf("  + telegram counter: %d\n", tel_counter);
            printf("  + scan counter: %d\n", scan_counter);
            printf("  + time since startup: %d\n", time_since_startup);
            printf("  + time of transmission: %d\n", time_of_transmission);
            printf("  + digital input status: %d\n", dig_input_status);
            printf("  + digital output status: %d\n", dig_output_status);
            printf("  + scan frequency: %d\n", scan_freq);
            printf("  + measurement frequency: %d\n", measurement_freq);
            printf("  + amount of encoder: %d\n", amount_of_encoder);
            printf("  + encoder position: %d\n", encoder_position);
            printf("  + encoder speed: %d\n", encoder_speed);
            printf("  + amount of 16 bit channels: %d\n", amount_of_16bit_channels);
            printf("  + channel content name: %s\n", channel_content_name.c_str());
            printf("  + scale factor: %f\n", scale_factor);
            printf("  + scale factor offset: %f\n", scale_factor_offset);
            printf("  + start angle: %d\n", start_angle);
            printf("  + steps: %d\n", steps);
            printf("  + amount of data: %d\n", amount_of_data);
            printf("  + amount of 8 bit channels: %d\n", amount_of_8bit_channels);
            printf("  + output of position data: %d\n", position);
            printf("  + device name len: %d\n", device_name_len);
            printf("  + event info flag: %d\n", event_info_flag);
#endif            
            state = LMS_STATE_ETX;
             // ----------------------------------------------------------------------------------------
          } // command == LSM_COMMAND_SSN in "LMDscandata"
          else {
            printf("ERROR: we're trying to handle LMDscandata but the command code is invalid.\n");
          }

        } // command_name == "LMDscandata"
        else {
          printf("- Unhandled command name: '%s', at: %ld\n", command_name.c_str(), bytes_flushed);
          return false;
        }

        command_name.clear(); 
        continue;
      } // LSM_STATE_COMMAND_PARAM


      case LMS_STATE_ETX: {

        if(data.size() == 0) {
          printf("- not enough data in buffer for LSM_STATE_ETX\n");
          return false;
        }
        if(data[0] != 0x03) {
          printf("- ERROR: something went wrong while parsing; we should have read a 0x03 value but read: %02X\n", data[0]);
          ::exit(0);
          return false;
        }

        flush(1); // space

        printf("\n");

        // start over
        state = LMS_STATE_NONE;
        continue;
      }

      default: {
        printf("Unhandled state.\n");
        return false;
      }

    };
  };

  return true;
}

std::string LMSParser::readHexNumberAsString(size_t maxChars, uint8_t stop) {

  if(maxChars > data.size()-1) {
    maxChars = data.size()-1;
  }

  if(maxChars == 0) {
    printf("ERROR: maxChars is 0\n");
    return "";
  }

  std::string res;
  for(size_t i = 0; i < maxChars; ++i) {
    if(data[i] == ' ' || data[i] == 0x03 || data[i] == stop) {
      return res;
    }
    res.push_back(data[i]);
  }

  return res;
}

uint8_t LMSParser::readHexNumberAsU8() {
  std::string hex_str = readHexNumberAsString(2);
  uint8_t result = 0;
  
  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> std::hex >> result;

  flush(hex_str.size());

  return result;

}

/*
// Optmized read; @todo implement + profile
template <typename TypeName>
unsigned hex2bin(TypeName c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return static_cast<unsigned>(-1);
}
*/

uint16_t LMSParser::readHexNumberAsU16(uint8_t stop) {
  std::string hex_str = readHexNumberAsString(4, stop);
  uint16_t result = 0;

  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> std::hex >> result;
  flush(hex_str.size());

  return result;
}

uint32_t LMSParser::readHexNumberAsU32(uint8_t stop) {
  std::string hex_str = readHexNumberAsString(8, stop);
  uint32_t result = 0;

  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> std::hex >> result;

  flush(hex_str.size());

  return result;
}

int32_t LMSParser::readHexNumberAsI32(uint8_t stop) {
  std::string hex_str = readHexNumberAsString(8, stop);
  int32_t result = 0;
  uint32_t tmp =  0;

  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> tmp;

  result = static_cast<int32_t>(tmp);

  flush(hex_str.size());

  return result;
}

int16_t LMSParser::readHexNumberAsI16(uint8_t stop) {
  std::string hex_str = readHexNumberAsString(4, stop);
  int16_t result = 0;
  uint16_t tmp =  0;

  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> tmp;

  result = static_cast<int16_t>(tmp);

  flush(hex_str.size());

  return result;
}

float LMSParser::readHexNumberAsFloat(uint8_t stop) {
  std::string hex_str = readHexNumberAsString(8, stop);
  float result = 0;
  float tmp =  0;

  std::stringstream ss;
  ss << std::hex << hex_str;
  ss >> tmp;

  result = static_cast<float>(tmp);

  flush(hex_str.size());

  return result;
}


void LMSParser::flush(size_t nbytes) {

  if(nbytes > data.size()) {
    printf("ERROR: trying to flush more bytes then there are in the buffer!\n");
    return;
  }

  bytes_flushed += nbytes;

  data.erase(data.begin(), data.begin() + nbytes);
}

void LMSParser::println() {
  size_t m = 40;
  if(data.size() < 40) {
    m = data.size();
  }

  printf("\n\n");
  for(size_t i = 0; i < m; ++i) {
    printf("%02X ", data[i]);
  }
  printf("\n");
  for(size_t i = 0; i < m; ++i) {
    printf("%2c ", (char)data[i]);
  }
  printf("\n\n");

}

std::string LMSParser::stlmsStatusCodeToString(uint16_t code) {
  switch(code) {
    case 0: return "undefined"; break;
    case 1: return "initialization"; break;
    case 2: return "configuration"; break;
    case 3: return "lowercase"; break;
    case 4: return "rotating"; break;
    case 5: return "in preparation"; break;
    case 6: return "ready"; break;
    case 7: return "measurement active"; break;
    case 8:
    case 9:
    case 10:
    case 11: return "reserved"; break;
    default: return "UNKNOWN"; break;
  }
}

