#include <openssl/sha.h>

#include <iostream>
#include <iosfwd>
#include <iomanip>
#include <sstream>
#include <vector>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "mpreal.h"
typedef mpfr::mpreal Float;
const int PRECISION = 300;

#define FLOAT(__f__) Float(__f__).setPrecision(PRECISION)

struct Hash256 {
  unsigned char value[SHA256_DIGEST_LENGTH];
};

Hash256 Hash(const void *p, size_t size) {
  Hash256 value;
  SHA256(reinterpret_cast<const unsigned char *>(p), size, value.value);
  return value;
}

std::string ConvertToHexStr(Hash256 value) {
  std::stringstream ss;
  ss << "0x";
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    ss << std::setfill('0') << std::setw(2) << std::hex << (int)value.value[i];
  }
  return ss.str();
}

Float Calculate_n(int bits) {
  return mpfr::pow(FLOAT(2), bits) * bits;
}

Float Calculate_1n(Float n) { return 1 / n; }

Float Calculate_2pow256() {
  return mpfr::pow(FLOAT(2), 256);
}

Float Calculate_quality(Hash256 val, int bits) {
  Float n = Calculate_n(bits);
  Float s(ConvertToHexStr(val), PRECISION, 16);;
  Float a = s / Calculate_2pow256();
  Float b = Calculate_1n(n);
  Float quality = mpfr::pow(a, b);
  Float rq = mpfr::pow(quality, n) * Calculate_2pow256();
  return quality;
}

int Calculate_w(Float q, const Hash256 &val, int num_of_samples = 128) {
  assert(num_of_samples >= 16);

  // Generate random values.
  std::vector<Hash256> vec_val;
  vec_val.push_back(val);
  Hash256 last_val = val;
  for (int i = 0; i < num_of_samples - 1; ++i) {
    Hash256 new_val = Hash(last_val.value, sizeof(last_val));
    vec_val.push_back(new_val);
    last_val = new_val;
  }

  // Qualities.
  int min_distance = num_of_samples, best_bits;
  for (int bits = 16; bits <= 64; ++bits) {
    int num_of_low_q = 0;
    for (const auto &val : vec_val) {
      Float this_q = Calculate_quality(val, bits);
      if (this_q < q) {
        ++num_of_low_q;
      }
    }
    int this_distance = std::abs(num_of_samples / 2 - num_of_low_q);
    if (this_distance < min_distance) {
      min_distance = this_distance;
      best_bits = bits;
    }
  }

  return best_bits;
}

std::tuple<Float, Float> Calculate_N(Float q) {
  Float N2 = 1 / mpfr::log2(1 / q);
  Float N3 = 1 / -mpfr::log2(q);
  return std::make_tuple(N2, N3);
}

struct Arguments {
  int bits;
  int samples;

  Arguments(int argc, const char *argv[]) {
    po::options_description opts;
    opts.add_options()  // All options
        ("bits,b", po::value(&bits)->default_value(32), "Set bits.")  // --bits
        ("samples,s", po::value(&samples)->default_value(128),
         "Number of samples for power of block.")  // --samples
        ;

    po::variables_map vars;
    po::store(po::parse_command_line(argc, argv, opts), vars);
    po::notify(vars);
  }
};

int main(int argc, const char *argv[]) {
  try {
    Arguments args(argc, argv);
    std::cout << "BITS=" << args.bits << std::endl;
    std::cout << "SAMPLES=" << args.samples << std::endl;
    std::cout << std::setprecision(PRECISION);
    time_t hash_src = time(NULL);
    Hash256 h256 = Hash(&hash_src, sizeof(hash_src));
    std::cout << "hash=" << ConvertToHexStr(h256) << std::endl;
    Float q = Calculate_quality(h256, args.bits);
    std::cout << "quality=" << q << std::endl;
    int w_bits = Calculate_w(q, h256, args.samples);
    std::cout << "w=" << w_bits << std::endl;
    Float N = mpfr::pow(FLOAT(2), FLOAT(w_bits)) * FLOAT(w_bits);
    std::cout << "N=" << N << std::endl;
    Float N2, N3;
    std::tie(N2, N3) = Calculate_N(q);
    std::cout << "N2=" << N2 << std::endl;
    std::cout << "N3=" << N3 << std::endl;
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  return 0;
}
