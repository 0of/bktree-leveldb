#ifndef HELPER_H
#define HELPER_H

struct Helper {
  static std::uint32_t parse(const std::string& value) {
    std::uint32_t i = 0;
    for (auto c : value) {
      i = (i << 4) | (c > 65 ? (10 + c - 65) : (c - 48));
    }
    return i;
  }

  static std::string stringfy(std::uint32_t value) {
    static char map[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    std::string v(8, '0');
    for (auto i = 0; i != 8; ++i) {
      v[i] = (map[(value >> (4 * (7 - i))) & 0xF]);
    }

    return std::move(v);
  }
};

#endif // HELPER_H