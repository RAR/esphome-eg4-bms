// Host-side test for the passive link splitter (components/eg4_modbus/eg4_modbus_frame.h).
//
//   make -C tests          # builds and runs
//
// eg4_link_frames.txt is a capture from the EG4-LL link port: the DIP-ID-1
// master polling slaves 2, 3 and 4 with FC03 0x0000 x45, one line per poll
// (request and, when the slave answered, its response concatenated).

#include "../components/eg4_modbus/eg4_modbus_frame.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace esphome::eg4_modbus;

static int failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                      \
    }                                                                  \
  } while (0)

static std::vector<uint8_t> hex_to_bytes(const std::string &hex) {
  std::vector<uint8_t> out;
  for (size_t i = 0; i + 1 < hex.size(); i += 2)
    out.push_back(static_cast<uint8_t>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16)));
  return out;
}

// Reads "(timestamp) rs232 HEX" lines and concatenates the bytes, as the UART
// would deliver them.
static std::vector<size_t> line_offsets;

static std::vector<uint8_t> load_capture(const char *path, size_t *lines) {
  std::ifstream in(path);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", path);
    std::exit(2);
  }
  std::vector<uint8_t> stream;
  std::string line;
  *lines = 0;
  while (std::getline(in, line)) {
    std::istringstream ss(line);
    std::string ts, tag, hex;
    if (!(ss >> ts >> tag >> hex))
      continue;
    auto bytes = hex_to_bytes(hex);
    line_offsets.push_back(stream.size());
    stream.insert(stream.end(), bytes.begin(), bytes.end());
    (*lines)++;
  }
  return stream;
}

struct Result {
  std::map<uint8_t, int> requests, responses;
  int unpaired_responses = 0;
  int bad_start = 0;
  int bad_len = 0;
  int other_frames = 0;
  uint32_t dropped = 0;
  size_t left_over = 0;
  std::vector<uint8_t> first_payload_addr3;
};

// Feeds the stream in uneven chunks (as UART reads would arrive) and pairs
// responses with requests exactly like EG4Modbus::handle_passive_frame_().
static Result run(const std::vector<uint8_t> &stream) {
  FrameSplitter splitter;
  RequestTracker tracker;
  Result r;
  static const size_t chunks[] = {1, 7, 64, 3, 33, 120, 5, 16};
  size_t pos = 0, k = 0;
  auto drain = [&](bool flush) {
    Frame f;
    while (splitter.next(f, flush)) {
      uint16_t start = MODBUS_START_REGISTER_UNKNOWN;
      const bool paired = tracker.pair(f, &start);
      if (f.type == FrameType::READ_REQUEST) {
        r.requests[f.address]++;
      } else if (f.type == FrameType::READ_RESPONSE) {
        if (!paired) {
          r.unpaired_responses++;
          continue;
        }
        r.responses[f.address]++;
        if (start != 0)
          r.bad_start++;
        if (f.payload_len != 90)
          r.bad_len++;
        if (f.address == 3 && r.first_payload_addr3.empty())
          r.first_payload_addr3.assign(f.payload, f.payload + f.payload_len);
      } else {
        r.other_frames++;
      }
    }
  };
  while (pos < stream.size()) {
    size_t n = std::min(chunks[k++ % (sizeof(chunks) / sizeof(chunks[0]))], stream.size() - pos);
    splitter.push(stream.data() + pos, n);
    pos += n;
    drain(false);
  }
  drain(true);  // line goes idle at the end of the capture
  r.dropped = splitter.dropped_bytes();
  r.left_over = splitter.buffered();
  return r;
}

static void test_crc() {
  const uint8_t req[] = {0x03, 0x03, 0x00, 0x00, 0x00, 0x2D, 0x84, 0x35};
  CHECK(crc16_modbus(req, 6) == 0x3584);
  CHECK(modbus_crc_ok(req, sizeof(req)));
}

static void test_capture(const std::vector<uint8_t> &stream, size_t lines) {
  Result r = run(stream);
  std::printf("capture: %zu lines, %zu bytes\n", lines, stream.size());
  for (uint8_t a : {2, 3, 4})
    std::printf("  addr %u: %d requests, %d paired responses\n", a, r.requests[a], r.responses[a]);
  std::printf("  unpaired=%d bad_start=%d bad_len=%d other=%d dropped=%u left=%zu\n", r.unpaired_responses,
              r.bad_start, r.bad_len, r.other_frames, (unsigned) r.dropped, r.left_over);

  int total_requests = 0;
  for (uint8_t a : {2, 3, 4}) {
    CHECK(r.requests[a] > 0);
    CHECK(r.responses[a] > 0);
    total_requests += r.requests[a];
  }
  CHECK((size_t) total_requests == lines);  // one request per poll line
  // The capture has 20/20/17 answers (pack 4 missed three polls).
  CHECK(r.responses[2] == 20);
  CHECK(r.responses[3] == 20);
  CHECK(r.responses[4] == 17);
  CHECK(r.requests.size() == 3);
  CHECK(r.unpaired_responses == 0);
  CHECK(r.bad_start == 0);
  CHECK(r.bad_len == 0);
  CHECK(r.other_frames == 0);
  CHECK(r.dropped == 0);
  CHECK(r.left_over == 0);

  // Spot-check the decode of the first answer from pack 3 against the map.
  const auto &p = r.first_payload_addr3;
  CHECK(p.size() == 90);
  if (p.size() == 90) {
    auto reg = [&](int i) { return (p[i * 2] << 8) | p[i * 2 + 1]; };
    CHECK(reg(0x00) == 0x14BB);                                         // 53.07 V
    CHECK((int16_t) reg(0x01) == -660);                                 // -6.60 A
    CHECK(reg(0x18) == 71);                                             // SOC
    CHECK(((uint32_t) reg(0x1D) << 16 | reg(0x1E)) == 298);              // cycles
    CHECK(((uint32_t) reg(0x1F) << 16 | reg(0x20)) / 3600000 == 280);    // full capacity Ah
    CHECK(reg(0x24) == 16);                                             // cell count
    CHECK(reg(0x25) == 2800);                                           // design capacity 280.0 Ah
  }
}

static void test_resync(const std::vector<uint8_t> &clean) {
  const Result baseline = run(clean);

  // Garbage that mimics frame starts: a valid address + FC03 with a large and
  // an odd byte count, a lone FC16 header, a reserved address, a bogus
  // exception, and a truncated copy of a real request.
  const std::vector<uint8_t> garbage = {0xAA, 0x03, 0x5A, 0x14, 0xBB, 0x00, 0xFF, 0x03, 0x03, 0x07, 0x02,
                                        0x10, 0x00, 0x00, 0xF9, 0x02, 0x83, 0x00, 0x03, 0x03, 0x00, 0x00,
                                        0x00, 0x2D, 0x84};

  std::vector<uint8_t> prefixed = garbage;
  prefixed.insert(prefixed.end(), clean.begin(), clean.end());
  Result r = run(prefixed);
  std::printf("resync (prefix): dropped=%u responses=%d/%d/%d\n", (unsigned) r.dropped, r.responses[2],
              r.responses[3], r.responses[4]);
  CHECK(r.dropped == garbage.size());
  for (uint8_t a : {2, 3, 4})
    CHECK(r.responses[a] == baseline.responses.at(a));
  CHECK(r.unpaired_responses == 0);
  CHECK(r.bad_start == 0);
  CHECK(r.bad_len == 0);
  CHECK(r.left_over == 0);

  // Garbage injected between polls in the middle of the stream, and a
  // corrupted response: only that one response may be lost.
  const size_t cut = line_offsets.at(5);
  std::vector<uint8_t> middle(clean.begin(), clean.begin() + cut);
  middle.insert(middle.end(), garbage.begin(), garbage.end());
  middle.insert(middle.end(), clean.begin() + cut, clean.end());
  CHECK(middle[line_offsets.at(2)] == 0x02);
  middle[line_offsets.at(2) + 40] ^= 0x01;  // inside the 3rd line's response (pack 2)
  r = run(middle);
  int total = r.responses[2] + r.responses[3] + r.responses[4];
  std::printf("resync (middle + corrupt): dropped=%u responses=%d\n", (unsigned) r.dropped, total);
  CHECK(total == 57 - 1);
  CHECK(r.responses[2] == 19);
  CHECK(r.unpaired_responses == 0);
  CHECK(r.bad_start == 0);
  CHECK(r.bad_len == 0);
  CHECK(r.left_over == 0);

  // Trailing partial frame is abandoned once the line goes idle.
  std::vector<uint8_t> trailing = clean;
  trailing.insert(trailing.end(), {0x02, 0x03, 0x5A, 0x14});
  r = run(trailing);
  CHECK(r.left_over == 0);
  CHECK(r.dropped == 4);
}

static void test_other_frames() {
  FrameSplitter s;
  RequestTracker t;
  // FC03 request then exception 02 (illegal data address) from slave 5.
  uint8_t req[8] = {0x05, 0x03, 0x00, 0x69, 0x00, 0x0B, 0, 0};
  uint16_t c = crc16_modbus(req, 6);
  req[6] = c & 0xFF;
  req[7] = c >> 8;
  uint8_t exc[5] = {0x05, 0x83, 0x02, 0, 0};
  c = crc16_modbus(exc, 3);
  exc[3] = c & 0xFF;
  exc[4] = c >> 8;
  // FC16 write request (2 registers) and its echo response, slave 6.
  uint8_t wreq[13] = {0x06, 0x10, 0x00, 0x10, 0x00, 0x02, 0x04, 0x12, 0x34, 0x56, 0x78, 0, 0};
  c = crc16_modbus(wreq, 11);
  wreq[11] = c & 0xFF;
  wreq[12] = c >> 8;
  uint8_t wresp[8] = {0x06, 0x10, 0x00, 0x10, 0x00, 0x02, 0, 0};
  c = crc16_modbus(wresp, 6);
  wresp[6] = c & 0xFF;
  wresp[7] = c >> 8;

  s.push(req, sizeof(req));
  s.push(exc, sizeof(exc));
  s.push(wreq, sizeof(wreq));
  s.push(wresp, sizeof(wresp));

  Frame f;
  uint16_t start = 0;
  CHECK(s.next(f) && f.type == FrameType::READ_REQUEST && f.start_register == 0x69 && f.quantity == 11);
  CHECK(!t.pair(f, &start));
  CHECK(s.next(f) && f.type == FrameType::EXCEPTION && f.payload_len == 1 && f.payload[0] == 0x02);
  CHECK(t.pair(f, &start) && start == 0x69);
  CHECK(s.next(f) && f.type == FrameType::WRITE_REQUEST && f.quantity == 2 && f.payload_len == 4);
  CHECK(!t.pair(f, &start));
  CHECK(s.next(f) && f.type == FrameType::WRITE_RESPONSE && f.start_register == 0x10);
  CHECK(t.pair(f, &start) && start == 0x10);
  CHECK(!s.next(f));
  CHECK(s.dropped_bytes() == 0);
}

static void test_bounded() {
  FrameSplitter s(512);
  // A stream of a plausible-looking header byte pattern that never completes.
  for (int i = 0; i < 10000; i++) {
    s.push(static_cast<uint8_t>(i & 1 ? 0x03 : 0x01));
    Frame f;
    while (s.next(f)) {
    }
    CHECK(s.buffered() <= 512);
  }
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "eg4_link_frames.txt";
  size_t lines = 0;
  const std::vector<uint8_t> stream = load_capture(path, &lines);

  test_crc();
  test_capture(stream, lines);
  test_resync(stream);
  test_other_frames();
  test_bounded();

  if (failures) {
    std::printf("FAILED: %d check(s)\n", failures);
    return 1;
  }
  std::printf("All passive link tests passed\n");
  return 0;
}
