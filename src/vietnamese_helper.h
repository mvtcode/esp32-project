#ifndef VIETNAMESE_HELPER_H
#define VIETNAMESE_HELPER_H

#include <Arduino.h>

// Map Unicode codepoint to custom character code (128 + index)
struct UnicodeMapping {
  uint32_t codepoint;
  uint8_t custom_code;
};

const UnicodeMapping unicodeMap[] = {
  { 0x00E1, 128 }, // á
  { 0x00E0, 129 }, // à
  { 0x1EA3, 130 }, // ả
  { 0x00E3, 131 }, // ã
  { 0x1EA1, 132 }, // ạ
  { 0x00E2, 133 }, // â
  { 0x1EA5, 134 }, // ấ
  { 0x1EA7, 135 }, // ầ
  { 0x1EA9, 136 }, // ẩ
  { 0x1EAB, 137 }, // ẫ
  { 0x1EAD, 138 }, // ậ
  { 0x0103, 139 }, // ă
  { 0x1EAF, 140 }, // ắ
  { 0x1EB1, 141 }, // ằ
  { 0x1EB3, 142 }, // ẳ
  { 0x1EB5, 143 }, // ẵ
  { 0x1EB7, 144 }, // ặ
  { 0x00E9, 145 }, // é
  { 0x00E8, 146 }, // è
  { 0x1EBB, 147 }, // ẻ
  { 0x1EBD, 148 }, // ẽ
  { 0x1EB9, 149 }, // ẹ
  { 0x00EA, 150 }, // ê
  { 0x1EBF, 151 }, // ế
  { 0x1EC1, 152 }, // ề
  { 0x1EC3, 153 }, // ể
  { 0x1EC5, 154 }, // ễ
  { 0x1EC7, 155 }, // ệ
  { 0x00ED, 156 }, // í
  { 0x00EC, 157 }, // ì
  { 0x1EC9, 158 }, // ỉ
  { 0x0129, 159 }, // ĩ
  { 0x1ECB, 160 }, // ị
  { 0x00F3, 161 }, // ó
  { 0x00F2, 162 }, // ò
  { 0x1ECF, 163 }, // ỏ
  { 0x00F5, 164 }, // õ
  { 0x1ECD, 165 }, // ọ
  { 0x00F4, 166 }, // ô
  { 0x1ED1, 167 }, // ố
  { 0x1ED3, 168 }, // ồ
  { 0x1ED5, 169 }, // ổ
  { 0x1ED7, 170 }, // ỗ
  { 0x1ED9, 171 }, // ộ
  { 0x01A1, 172 }, // ơ
  { 0x1EDB, 173 }, // ớ
  { 0x1EDD, 174 }, // ờ
  { 0x1EDF, 175 }, // ở
  { 0x1EE1, 176 }, // ỡ
  { 0x1EE3, 177 }, // ợ
  { 0x00FA, 178 }, // ú
  { 0x00F9, 179 }, // ù
  { 0x1EE7, 180 }, // ủ
  { 0x0169, 181 }, // ũ
  { 0x1EE5, 182 }, // ụ
  { 0x01B0, 183 }, // ư
  { 0x1EE9, 184 }, // ứ
  { 0x1EEB, 185 }, // ừ
  { 0x1EED, 186 }, // ử
  { 0x1EEF, 187 }, // ữ
  { 0x1EF1, 188 }, // ự
  { 0x00FD, 189 }, // ý
  { 0x1EF3, 190 }, // ỳ
  { 0x1EF7, 191 }, // ỷ
  { 0x1EF9, 192 }, // ỹ
  { 0x1EF5, 193 }, // ỵ
  { 0x0111, 194 }, // đ
  { 0x00C1, 195 }, // Á
  { 0x00C0, 196 }, // À
  { 0x1EA2, 197 }, // Ả
  { 0x1EA0, 198 }, // Ạ
  { 0x00C2, 199 }, // Â
  { 0x1EA4, 200 }, // Ấ
  { 0x1EAC, 201 }, // Ậ
  { 0x0102, 202 }, // Ă
  { 0x1EAE, 203 }, // Ắ
  { 0x1EB6, 204 }, // Ặ
  { 0x00C9, 205 }, // É
  { 0x00C8, 206 }, // È
  { 0x1EBA, 207 }, // Ẻ
  { 0x00CA, 208 }, // Ê
  { 0x1EBE, 209 }, // Ế
  { 0x1EC6, 210 }, // Ệ
  { 0x00CD, 211 }, // Í
  { 0x00CC, 212 }, // Ì
  { 0x1ECA, 213 }, // Ị
  { 0x00D3, 214 }, // Ó
  { 0x00D2, 215 }, // Ò
  { 0x1ECE, 216 }, // Ỏ
  { 0x00D4, 217 }, // Ô
  { 0x1ED0, 218 }, // Ố
  { 0x1ED8, 219 }, // Ộ
  { 0x01A0, 220 }, // Ơ
  { 0x1EDA, 221 }, // Ớ
  { 0x1EE2, 222 }, // Ợ
  { 0x00DA, 223 }, // Ú
  { 0x00D9, 224 }, // Ù
  { 0x1EE6, 225 }, // Ủ
  { 0x01AF, 226 }, // Ư
  { 0x1EE8, 227 }, // Ứ
  { 0x1EF0, 228 }, // Ự
  { 0x00DD, 229 }, // Ý
  { 0x1EF2, 230 }, // Ỳ
  { 0x1EF4, 231 }, // Ỵ
  { 0x0110, 232 }, // Đ
  // Fallback mappings for missing uppercase characters to lowercase equivalents (since uppercase glyphs above 232 are not defined in the font files)
  { 0x1EA6, 135 }, // Ầ -> ầ
  { 0x1EA8, 136 }, // Ẩ -> ẩ
  { 0x1EAA, 137 }, // Ẫ -> ẫ
  { 0x1EC0, 152 }, // Ề -> ề
  { 0x1EC2, 153 }, // Ể -> ể
  { 0x1EC4, 154 }, // Ễ -> ễ
  { 0x1ED2, 168 }, // Ồ -> ồ
  { 0x1ED4, 169 }, // Ổ -> ổ
  { 0x1ED6, 170 }, // Ỗ -> ỗ
  { 0x1EEC, 185 }, // Ừ -> ừ
  { 0x1EEE, 186 }, // Ử -> ử
  { 0x1EF0, 187 }, // Ữ -> ữ
};

const int unicodeMapSize = sizeof(unicodeMap) / sizeof(UnicodeMapping);

// Decode UTF-8 string to custom 8-bit string
inline String utf8ToCustom(const String& utf8_str) {
  String result = "";
  int i = 0;
  int len = utf8_str.length();
  
  while (i < len) {
    uint32_t codepoint = 0;
    uint8_t c = utf8_str[i];
    
    if (c < 0x80) {
      codepoint = c;
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      if (i + 1 < len) {
        codepoint = ((c & 0x1F) << 6) | (utf8_str[i+1] & 0x3F);
        i += 2;
      } else {
        codepoint = c;
        i += 1;
      }
    } else if ((c & 0xF0) == 0xE0) {
      if (i + 2 < len) {
        codepoint = ((c & 0x0F) << 12) | ((utf8_str[i+1] & 0x3F) << 6) | (utf8_str[i+2] & 0x3F);
        i += 3;
      } else {
        codepoint = c;
        i += 1;
      }
    } else if ((c & 0xF8) == 0xF0) {
      if (i + 3 < len) {
        codepoint = ((c & 0x07) << 18) | ((utf8_str[i+1] & 0x3F) << 12) | ((utf8_str[i+2] & 0x3F) << 6) | (utf8_str[i+3] & 0x3F);
        i += 4;
      } else {
        codepoint = c;
        i += 1;
      }
    } else {
      codepoint = c;
      i += 1;
    }
    
    // Look up codepoint in map
    if (codepoint < 128) {
      result += (char)codepoint;
    } else {
      bool found = false;
      for (int m = 0; m < unicodeMapSize; m++) {
        if (unicodeMap[m].codepoint == codepoint) {
          result += (char)unicodeMap[m].custom_code;
          found = true;
          break;
        }
      }
      if (!found) {
        result += '?';
      }
    }
  }
  return result;
}

#endif // VIETNAMESE_HELPER_H
