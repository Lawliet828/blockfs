#include "negotiation.h"

#include "file_store_udisk.h"
#include "logging.h"

Negotiation::Negotiation() {}

Negotiation::~Negotiation() {}

bool Negotiation::InitializeMeta() {
  NegotMeta *meta = this->meta();
  uint32_t crc =
      Crc32(reinterpret_cast<uint8_t *>(base_addr()) + sizeof(meta->crc_),
            kNegotiationSize - sizeof(meta->crc_));
  if (unlikely(meta->crc_ != crc)) {
    LOG(ERROR) << "negotiation crc error, read:" << meta->crc_
               << " cal: " << crc;
    return false;
  }
  LOG(DEBUG) << "read negotiation success";
  return true;
}

bool Negotiation::FormatAllMeta() {
  if (!block_fs_is_master()) {
    return false;
  }
  AlignBufferPtr buffer = std::make_shared<AlignBuffer>(
      kNegotiationSize, FileStore::Instance()->dev()->block_size());
  NegotMeta *meta = reinterpret_cast<NegotMeta *>(buffer->data());
  meta->crc_ =
      Crc32(reinterpret_cast<uint8_t *>(buffer->data()) + sizeof(meta->crc_),
            kNegotiationSize - sizeof(meta->crc_));
  int64_t ret = FileStore::Instance()->dev()->PwriteDirect(
      buffer->data(), kNegotiationSize, kNegotiationOffset);
  if (ret != kNegotiationSize) {
    LOG(ERROR) << "write negotiation error size:" << ret;
    return false;
  }
  LOG(INFO) << "write all negotiation success";
  return true;
}

void Negotiation::Dump() noexcept {}

void Negotiation::Dump(const std::string &file_name) noexcept {}