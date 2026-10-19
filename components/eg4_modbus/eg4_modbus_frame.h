#pragma once

// Modbus RTU framing helpers shared by the eg4_modbus hub and the host-side
// tests in tests/. Plain C++ only - no ESPHome includes - so the passive link
// splitter can be exercised with g++ on any machine.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace esphome {
namespace eg4_modbus {

constexpr uint8_t MODBUS_FC_READ_HOLDING_REGISTERS = 0x03;
constexpr uint8_t MODBUS_FC_WRITE_MULTIPLE_REGISTERS = 0x10;
constexpr uint8_t MODBUS_EXCEPTION_FLAG = 0x80;

// Passed as start_register when a response cannot be paired with a request.
constexpr uint16_t MODBUS_START_REGISTER_UNKNOWN = 0xFFFF;

inline uint16_t crc16_modbus(const uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// True when the last two bytes of `frame` are the (little-endian) CRC of the rest.
inline bool modbus_crc_ok(const uint8_t *frame, size_t len) {
  if (len < 4)
    return false;
  const uint16_t calc = crc16_modbus(frame, static_cast<uint16_t>(len - 2));
  const uint16_t recv = static_cast<uint16_t>(frame[len - 2] | (frame[len - 1] << 8));
  return calc == recv;
}

enum class FrameType : uint8_t {
  READ_REQUEST,    // addr 03 start(2) qty(2) crc
  READ_RESPONSE,   // addr 03 n data(n) crc
  WRITE_REQUEST,   // addr 10 start(2) qty(2) n data(n) crc
  WRITE_RESPONSE,  // addr 10 start(2) qty(2) crc
  EXCEPTION,       // addr fc|80 code crc
};

struct Frame {
  FrameType type{FrameType::READ_REQUEST};
  uint8_t address{0};
  uint8_t function{0};  // as on the wire, exceptions keep the 0x80 bit
  uint16_t start_register{MODBUS_START_REGISTER_UNKNOWN};  // requests and FC16 responses only
  uint16_t quantity{0};  // registers; for a read response this is n / 2
  // Read response: register data. Write request: register data. Exception:
  // the exception code. Otherwise nullptr. Valid until the next call to
  // FrameSplitter::next().
  const uint8_t *payload{nullptr};
  size_t payload_len{0};
  const uint8_t *raw{nullptr};
  size_t raw_len{0};
};

// Splits a byte stream that carries both directions of a Modbus RTU link into
// frames by structure and CRC alone. Inter-frame gaps cannot be relied on: on
// the EG4 link port a slave answers within 1-2 ms of the master's request, far
// less than one ESPHome loop iteration, so request and response routinely land
// in the same read.
//
// At the head of the buffer every frame shape the function code allows is
// tried (FC03 request/response, FC16 request/response, exception). A shape
// whose CRC matches is emitted and consumed. If no shape matches and none can
// still complete with more data, one byte is dropped and the scan restarts at
// the next byte (resync).
class FrameSplitter {
 public:
  static constexpr size_t MAX_FRAME_LEN = 256;

  explicit FrameSplitter(size_t max_buffered = 512)
      : max_buffered_(max_buffered < MAX_FRAME_LEN ? MAX_FRAME_LEN : max_buffered) {
    this->frame_buf_.reserve(MAX_FRAME_LEN);
  }

  void push(uint8_t byte) {
    // Keep the buffer bounded: a frame is never longer than MAX_FRAME_LEN, so
    // anything older than max_buffered_ bytes can no longer start one.
    if (this->buffered() >= this->max_buffered_) {
      this->head_++;
      this->dropped_++;
    }
    this->buf_.push_back(byte);
    this->compact_();
  }

  void push(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++)
      this->push(data[i]);
  }

  // Extracts the next frame. Returns false when the buffer holds no complete
  // frame. With `flush` set (the line has gone idle) a partial candidate that
  // is still waiting for bytes is abandoned and the scan moves past it, so
  // leading garbage cannot hold back the frames behind it indefinitely.
  bool next(Frame &frame, bool flush = false) {
    while (this->buffered() > 0) {
      const uint8_t *p = this->buf_.data() + this->head_;
      size_t frame_len = 0;
      Frame candidate;
      const Match m = match_at_(p, this->buffered(), candidate, frame_len);
      if (m == Match::FRAME) {
        this->frame_buf_.assign(p, p + frame_len);
        const uint8_t *base = this->frame_buf_.data();
        if (candidate.payload != nullptr)
          candidate.payload = base + (candidate.payload - p);
        candidate.raw = base;
        candidate.raw_len = frame_len;
        this->head_ += frame_len;
        this->compact_();
        frame = candidate;
        return true;
      }
      if (m == Match::NEED_MORE && !flush)
        return false;
      this->head_++;
      this->dropped_++;
    }
    this->compact_();
    return false;
  }

  size_t buffered() const { return this->buf_.size() - this->head_; }
  // Bytes discarded while resyncing or because the buffer overflowed.
  uint32_t dropped_bytes() const { return this->dropped_; }
  void clear() {
    this->buf_.clear();
    this->head_ = 0;
  }

 protected:
  enum class Match { NONE, NEED_MORE, FRAME };

  // Outcome of checking one candidate frame length at the buffer head.
  enum class Check { INVALID, PENDING, VALID };

  static Check check_(const uint8_t *p, size_t avail, size_t len) {
    if (avail < len)
      return Check::PENDING;
    return modbus_crc_ok(p, len) ? Check::VALID : Check::INVALID;
  }

  static uint16_t be16_(const uint8_t *p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

  static Match match_at_(const uint8_t *p, size_t avail, Frame &f, size_t &len) {
    // 0x00 is broadcast (never answered) and 0xF8-0xFF are reserved.
    if (p[0] == 0x00 || p[0] > 0xF7)
      return Match::NONE;
    if (avail < 2)
      return Match::NEED_MORE;

    f.address = p[0];
    f.function = p[1];
    bool pending = false;

    switch (p[1]) {
      case MODBUS_FC_READ_HOLDING_REGISTERS: {
        // Response: addr 03 n data(n) crc, n even and non-zero.
        Check resp = Check::PENDING;
        size_t resp_len = 0;
        if (avail >= 3) {
          const uint8_t n = p[2];
          if (n == 0 || (n & 1) != 0 || n > 250) {
            resp = Check::INVALID;
          } else {
            resp_len = 5u + n;
            resp = check_(p, avail, resp_len);
          }
        }
        // Request: addr 03 start(2) qty(2) crc, qty 1..125.
        Check req = Check::PENDING;
        if (avail >= 6) {
          const uint16_t qty = be16_(p + 4);
          req = (qty == 0 || qty > 125) ? Check::INVALID : check_(p, avail, 8);
        }
        // Both CRCs matching needs a 1-in-65536 coincidence; prefer the
        // response, whose length is self-describing.
        if (resp == Check::VALID) {
          f.type = FrameType::READ_RESPONSE;
          f.quantity = p[2] / 2;
          f.payload = p + 3;
          f.payload_len = p[2];
          len = resp_len;
          return Match::FRAME;
        }
        if (req == Check::VALID) {
          f.type = FrameType::READ_REQUEST;
          f.start_register = be16_(p + 2);
          f.quantity = be16_(p + 4);
          len = 8;
          return Match::FRAME;
        }
        pending = resp == Check::PENDING || req == Check::PENDING;
        break;
      }

      case MODBUS_FC_WRITE_MULTIPLE_REGISTERS: {
        // Request: addr 10 start(2) qty(2) n data(n) crc, n == 2 * qty.
        Check req = Check::PENDING;
        size_t req_len = 0;
        if (avail >= 7) {
          const uint16_t qty = be16_(p + 4);
          const uint8_t n = p[6];
          if (qty == 0 || qty > 123 || n != qty * 2) {
            req = Check::INVALID;
          } else {
            req_len = 9u + n;
            req = check_(p, avail, req_len);
          }
        }
        // Response: addr 10 start(2) qty(2) crc.
        Check resp = Check::PENDING;
        if (avail >= 6) {
          const uint16_t qty = be16_(p + 4);
          resp = (qty == 0 || qty > 123) ? Check::INVALID : check_(p, avail, 8);
        }
        if (req == Check::VALID) {
          f.type = FrameType::WRITE_REQUEST;
          f.start_register = be16_(p + 2);
          f.quantity = be16_(p + 4);
          f.payload = p + 7;
          f.payload_len = p[6];
          len = req_len;
          return Match::FRAME;
        }
        if (resp == Check::VALID) {
          f.type = FrameType::WRITE_RESPONSE;
          f.start_register = be16_(p + 2);
          f.quantity = be16_(p + 4);
          len = 8;
          return Match::FRAME;
        }
        pending = resp == Check::PENDING || req == Check::PENDING;
        break;
      }

      case MODBUS_FC_READ_HOLDING_REGISTERS | MODBUS_EXCEPTION_FLAG:
      case MODBUS_FC_WRITE_MULTIPLE_REGISTERS | MODBUS_EXCEPTION_FLAG: {
        // Exception: addr fc|80 code crc, code 1..11.
        if (avail >= 3 && (p[2] == 0 || p[2] > 0x0B))
          return Match::NONE;
        const Check exc = check_(p, avail, 5);
        if (exc == Check::VALID) {
          f.type = FrameType::EXCEPTION;
          f.payload = p + 2;
          f.payload_len = 1;
          len = 5;
          return Match::FRAME;
        }
        pending = exc == Check::PENDING;
        break;
      }

      default:
        return Match::NONE;
    }

    return pending ? Match::NEED_MORE : Match::NONE;
  }

  void compact_() {
    if (this->head_ == 0)
      return;
    if (this->head_ >= this->buf_.size()) {
      this->buf_.clear();
      this->head_ = 0;
    } else if (this->head_ >= MAX_FRAME_LEN) {
      this->buf_.erase(this->buf_.begin(), this->buf_.begin() + this->head_);
      this->head_ = 0;
    }
  }

  std::vector<uint8_t> buf_;
  size_t head_{0};
  std::vector<uint8_t> frame_buf_;
  size_t max_buffered_;
  uint32_t dropped_{0};
};

// Remembers the most recent request per slave address so a response, which
// does not carry its start register, can be attributed to a register block.
// Used by both modes: active mode records what it sent, passive mode records
// what the bus master sent.
class RequestTracker {
 public:
  void on_request(uint8_t address, uint8_t function, uint16_t start_register, uint16_t quantity) {
    Entry *slot = nullptr;
    for (auto &e : this->entries_) {
      if (e.used && e.address == address) {
        slot = &e;
        break;
      }
    }
    if (slot == nullptr) {
      for (auto &e : this->entries_) {
        if (!e.used) {
          slot = &e;
          break;
        }
      }
    }
    if (slot == nullptr) {
      // Evict the least recently used address.
      slot = &this->entries_[0];
      for (auto &e : this->entries_) {
        if (e.seq < slot->seq)
          slot = &e;
      }
    }
    slot->used = true;
    slot->address = address;
    slot->function = function;
    slot->start_register = start_register;
    slot->quantity = quantity;
    slot->seq = ++this->seq_;
  }

  // Pairs a response with the outstanding request for its address and consumes
  // that request. `function` may carry the exception bit. `quantity` is the
  // register count the response carries, or 0 to accept any (exceptions).
  bool take(uint8_t address, uint8_t function, uint16_t quantity, uint16_t *start_register) {
    const uint8_t base = function & static_cast<uint8_t>(~MODBUS_EXCEPTION_FLAG);
    for (auto &e : this->entries_) {
      if (!e.used || e.address != address)
        continue;
      if (e.function != base || (quantity != 0 && e.quantity != quantity))
        return false;
      e.used = false;
      if (start_register != nullptr)
        *start_register = e.start_register;
      return true;
    }
    return false;
  }

  // Feeds one frame from a passively observed bus. Requests are recorded and
  // return false. Responses and exceptions return true with the start
  // register of the request they answer, or false when none was seen.
  bool pair(const Frame &frame, uint16_t *start_register) {
    switch (frame.type) {
      case FrameType::READ_REQUEST:
      case FrameType::WRITE_REQUEST:
        this->on_request(frame.address, frame.function, frame.start_register, frame.quantity);
        return false;
      case FrameType::READ_RESPONSE:
        return this->take(frame.address, frame.function, frame.quantity, start_register);
      case FrameType::WRITE_RESPONSE:
        return this->take(frame.address, frame.function, frame.quantity, start_register);
      case FrameType::EXCEPTION:
        return this->take(frame.address, frame.function, 0, start_register);
    }
    return false;
  }

 protected:
  struct Entry {
    bool used{false};
    uint8_t address{0};
    uint8_t function{0};
    uint16_t start_register{0};
    uint16_t quantity{0};
    uint32_t seq{0};
  };
  static constexpr size_t MAX_ENTRIES = 16;
  Entry entries_[MAX_ENTRIES];
  uint32_t seq_{0};
};

}  // namespace eg4_modbus
}  // namespace esphome
