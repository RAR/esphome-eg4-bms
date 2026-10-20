// Host-side test for the RS232 service port (components/eg4_bms/eg4_bms_settings.h
// and the shared Modbus framing code).
//
//   make -C tests          # builds and runs
//
// rs232_service_frames.txt holds the replies of a DIP-ID-1 pack on its RS232
// service port (115200 8N1) to the six reads eg4_bms makes there, captured with
// an AtomS3 + RS232 base on 2026-10-20. Each request line was rebuilt from the
// read that was sent and placed 10 ms before its reply; the replies are as
// captured.

#include "../components/eg4_modbus/eg4_modbus_frame.h"
#include "../components/eg4_bms/eg4_bms_settings.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace esphome::eg4_modbus;
using namespace esphome::eg4_bms;

static int failures = 0;

#define CHECK(cond)                                                                 \
  do {                                                                              \
    if (!(cond)) {                                                                  \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                                   \
    }                                                                               \
  } while (0)

static std::vector<uint8_t> hex_to_bytes(const std::string &hex) {
  std::vector<uint8_t> out;
  for (size_t i = 0; i + 1 < hex.size(); i += 2)
    out.push_back(static_cast<uint8_t>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16)));
  return out;
}

static std::vector<uint8_t> load_capture(const char *path) {
  std::ifstream in(path);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", path);
    std::exit(2);
  }
  std::vector<uint8_t> stream;
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream ss(line);
    std::string ts, tag, hex;
    if (!(ss >> ts >> tag >> hex))
      continue;
    auto bytes = hex_to_bytes(hex);
    stream.insert(stream.end(), bytes.begin(), bytes.end());
  }
  return stream;
}

static uint16_t be16(const std::vector<uint8_t> &p, uint16_t start, uint16_t reg) {
  const size_t i = (size_t) (reg - start) * 2;
  return (uint16_t(p[i]) << 8) | p[i + 1];
}

// Splits the stream and pairs every reply with its request, as the hub does.
static std::map<uint16_t, std::vector<uint8_t>> blocks_by_start(const std::vector<uint8_t> &stream) {
  FrameSplitter splitter;
  RequestTracker tracker;
  std::map<uint16_t, std::vector<uint8_t>> blocks;
  splitter.push(stream.data(), stream.size());
  Frame f;
  while (splitter.next(f, true)) {
    uint16_t start = MODBUS_START_REGISTER_UNKNOWN;
    const bool paired = tracker.pair(f, &start);
    if (f.type == FrameType::READ_RESPONSE) {
      CHECK(paired);
      CHECK(f.address == 1);
      blocks[start].assign(f.payload, f.payload + f.payload_len);
    }
  }
  CHECK(splitter.dropped_bytes() == 0);
  return blocks;
}

static void test_blocks(const std::map<uint16_t, std::vector<uint8_t>> &blocks) {
  CHECK(blocks.size() == 6);
  CHECK(blocks.count(0x00) && blocks.at(0x00).size() == 0x12 * 2);
  CHECK(blocks.count(0x12) && blocks.at(0x12).size() == 0x15 * 2);
  CHECK(blocks.count(0x69) && blocks.at(0x69).size() == 11 * 2);
  CHECK(blocks.count(0x75) && blocks.at(0x75).size() == 3 * 2);
  CHECK(blocks.count(0x78) && blocks.at(0x78).size() == 8 * 2);
  CHECK(blocks.count(REG_SETTINGS) && blocks.at(REG_SETTINGS).size() == SETTINGS_REGISTERS * 2);
  if (!blocks.count(REG_SETTINGS) || !blocks.count(0x69) || !blocks.count(0x00))
    return;

  const auto &live = blocks.at(0x00);
  CHECK(be16(live, 0x00, 0x00) == 5330);  // 53.30 V
  CHECK(be16(live, 0x00, 0x02) == 3330);  // cell 1, mV

  const auto &model = blocks.at(0x69);
  CHECK(std::string(model.begin(), model.begin() + 21) == "ZTR-16S200A-10KW-STD2");

  // The parameter block carries the same text as the 0x69/0x75/0x78 reads.
  const auto &s = blocks.at(REG_SETTINGS);
  const size_t text_at = (0x69 - REG_SETTINGS) * 2;
  CHECK(std::memcmp(&s[text_at], model.data(), model.size()) == 0);

  // A few thresholds the BMS tool shows for this pack.
  CHECK(be16(s, REG_SETTINGS, 0x38) == 3400);   // balance start, mV
  CHECK(be16(s, REG_SETTINGS, 0x3E) == 2300);   // cell UV protection, mV
  CHECK(be16(s, REG_SETTINGS, 0x47) == 6000);   // pack OV protection, 10 mV
  CHECK(be16(s, REG_SETTINGS, 0x50) == 20500);  // charge OC1, 10 mA
  CHECK(be16(s, REG_SETTINGS, 0x5B) == 45);     // charge UT protection, degC + 50
  CHECK(be16(s, REG_SETTINGS, 0x87) == 110);    // ambient OT release, degC + 50

  CHECK(!thresholds_blank(REG_SETTINGS, s.data(), s.size()));

  // The paralleling bus answers the same read with the thresholds zero but the
  // clock and text intact.
  std::vector<uint8_t> blank = s;
  for (uint16_t reg = REG_SETTINGS; reg <= REG_SETTINGS_LAST; reg++) {
    if (is_threshold_register(reg)) {
      blank[(reg - REG_SETTINGS) * 2] = 0;
      blank[(reg - REG_SETTINGS) * 2 + 1] = 0;
    }
  }
  CHECK(thresholds_blank(REG_SETTINGS, blank.data(), blank.size()));

  // Blocks without threshold registers are never "blank".
  CHECK(!thresholds_blank(0x00, live.data(), live.size()));
  CHECK(!thresholds_blank(0x69, model.data(), model.size()));

  const uint32_t clock = (uint32_t(be16(s, REG_SETTINGS, 0x2D)) << 16) | be16(s, REG_SETTINGS, 0x2E);
  char buf[24];
  CHECK(format_bms_time(clock, buf, sizeof(buf)));
  CHECK(std::strcmp(buf, "2026-10-20 11:25:45") == 0);
}

// The BMS tool's own writes (bms_sw/write temp.txt): one FC16 per register, the
// BMS echoes start and quantity.
static void test_tool_writes() {
  std::vector<uint8_t> bytes = hex_to_bytes("011000380001020D48A64E0110003800018004");
  FrameSplitter splitter;
  RequestTracker tracker;
  splitter.push(bytes.data(), bytes.size());
  Frame f;
  int requests = 0, acks = 0;
  while (splitter.next(f, true)) {
    uint16_t start = MODBUS_START_REGISTER_UNKNOWN;
    const bool paired = tracker.pair(f, &start);
    if (f.type == FrameType::WRITE_REQUEST) {
      requests++;
      CHECK(f.start_register == 0x38);
      CHECK(f.quantity == 1);
    } else if (f.type == FrameType::WRITE_RESPONSE) {
      acks++;
      CHECK(paired);
      CHECK(start == 0x38);
    }
  }
  CHECK(requests == 1);
  CHECK(acks == 1);

  // The tool's clock write: 01 10 00 2D 00 02 04 67 11 07 FA.
  CHECK(pack_bms_time(2025, 12, 8, 16, 31, 58) == 0x671107FAu);
}

static void test_time() {
  char buf[24];
  // 2025-12-08 16:31:58, the sample in the BMS tool's own log.
  CHECK(format_bms_time(0x671107FA, buf, sizeof(buf)));
  CHECK(std::strcmp(buf, "2025-12-08 16:31:58") == 0);
  // Month 0 / a zeroed clock is not a time.
  CHECK(!format_bms_time(0, buf, sizeof(buf)));
  CHECK(!format_bms_time((25u << 26) | (13u << 22) | (1u << 17), buf, sizeof(buf)));

  // pack/format round trip, and pack refuses what format would reject.
  const uint32_t packed = pack_bms_time(2026, 10, 20, 23, 59, 7);
  CHECK(packed != 0);
  CHECK(format_bms_time(packed, buf, sizeof(buf)));
  CHECK(std::strcmp(buf, "2026-10-20 23:59:07") == 0);
  CHECK(pack_bms_time(1999, 1, 1, 0, 0, 0) == 0);
  CHECK(pack_bms_time(2064, 1, 1, 0, 0, 0) == 0);
  CHECK(pack_bms_time(2026, 0, 1, 0, 0, 0) == 0);
  CHECK(pack_bms_time(2026, 1, 1, 24, 0, 0) == 0);
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "rs232_service_frames.txt";
  test_blocks(blocks_by_start(load_capture(path)));
  test_time();
  test_tool_writes();
  if (failures) {
    std::printf("FAILED: %d check(s)\n", failures);
    return 1;
  }
  std::printf("All RS232 service port tests passed\n");
  return 0;
}
