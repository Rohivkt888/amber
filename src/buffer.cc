// Copyright 2018 The Amber Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/buffer.h"

#include <cassert>
#include <cmath>
#include <cstring>

namespace amber {
namespace {

// Return sign value of 32 bits float.
uint16_t FloatSign(const uint32_t hex_float) {
  return static_cast<uint16_t>(hex_float >> 31U);
}

// Return exponent value of 32 bits float.
uint16_t FloatExponent(const uint32_t hex_float) {
  uint32_t exponent = ((hex_float >> 23U) & ((1U << 8U) - 1U)) - 112U;
  const uint32_t half_exponent_mask = (1U << 5U) - 1U;
  assert(((exponent & ~half_exponent_mask) == 0U) && "Float exponent overflow");
  return static_cast<uint16_t>(exponent & half_exponent_mask);
}

// Return mantissa value of 32 bits float. Note that mantissa for 32
// bits float is 23 bits and this method must return uint32_t.
uint32_t FloatMantissa(const uint32_t hex_float) {
  return static_cast<uint32_t>(hex_float & ((1U << 23U) - 1U));
}

// Convert 32 bits float |value| to 16 bits float based on IEEE-754.
uint16_t FloatToHexFloat16(const float value) {
  const uint32_t* hex = reinterpret_cast<const uint32_t*>(&value);
  return static_cast<uint16_t>(
      static_cast<uint16_t>(FloatSign(*hex) << 15U) |
      static_cast<uint16_t>(FloatExponent(*hex) << 10U) |
      static_cast<uint16_t>(FloatMantissa(*hex) >> 13U));
}

template <typename T>
T* ValuesAs(uint8_t* values) {
  return reinterpret_cast<T*>(values);
}

template <typename T>
double Sub(const uint8_t* buf1, const uint8_t* buf2) {
  return static_cast<double>(*reinterpret_cast<const T*>(buf1) -
                             *reinterpret_cast<const T*>(buf2));
}

double CalculateDiff(const Format::Segment* seg,
                     const uint8_t* buf1,
                     const uint8_t* buf2) {
  FormatMode mode = seg->GetFormatMode();
  uint32_t num_bits = seg->GetNumBits();
  if (type::Type::IsInt8(mode, num_bits))
    return Sub<int8_t>(buf1, buf2);
  if (type::Type::IsInt16(mode, num_bits))
    return Sub<int16_t>(buf1, buf2);
  if (type::Type::IsInt32(mode, num_bits))
    return Sub<int32_t>(buf1, buf2);
  if (type::Type::IsInt64(mode, num_bits))
    return Sub<int64_t>(buf1, buf2);
  if (type::Type::IsUint8(mode, num_bits))
    return Sub<uint8_t>(buf1, buf2);
  if (type::Type::IsUint16(mode, num_bits))
    return Sub<uint16_t>(buf1, buf2);
  if (type::Type::IsUint32(mode, num_bits))
    return Sub<uint32_t>(buf1, buf2);
  if (type::Type::IsUint64(mode, num_bits))
    return Sub<uint64_t>(buf1, buf2);
  // TOOD(dsinclair): Handle float16 ...
  if (type::Type::IsFloat16(mode, num_bits)) {
    assert(false && "Float16 suppport not implemented");
    return 0.0;
  }
  if (type::Type::IsFloat32(mode, num_bits))
    return Sub<float>(buf1, buf2);
  if (type::Type::IsFloat64(mode, num_bits))
    return Sub<double>(buf1, buf2);

  assert(false && "NOTREACHED");
  return 0.0;
}

}  // namespace

Buffer::Buffer() = default;

Buffer::Buffer(BufferType type) : buffer_type_(type) {}

Buffer::~Buffer() = default;

Result Buffer::CopyTo(Buffer* buffer) const {
  if (buffer->width_ != width_)
    return Result("Buffer::CopyBaseFields() buffers have a different width");
  if (buffer->height_ != height_)
    return Result("Buffer::CopyBaseFields() buffers have a different height");
  if (buffer->element_count_ != element_count_)
    return Result("Buffer::CopyBaseFields() buffers have a different size");
  buffer->bytes_ = bytes_;
  return {};
}

Result Buffer::IsEqual(Buffer* buffer) const {
  if (!buffer->format_->Equal(format_))
    return Result{"Buffers have a different format"};
  if (buffer->element_count_ != element_count_)
    return Result{"Buffers have a different size"};
  if (buffer->width_ != width_)
    return Result{"Buffers have a different width"};
  if (buffer->height_ != height_)
    return Result{"Buffers have a different height"};
  if (buffer->bytes_.size() != bytes_.size())
    return Result{"Buffers have a different number of values"};

  uint32_t num_different = 0;
  uint32_t first_different_index = 0;
  uint8_t first_different_left = 0;
  uint8_t first_different_right = 0;
  for (uint32_t i = 0; i < bytes_.size(); ++i) {
    if (bytes_[i] != buffer->bytes_[i]) {
      if (num_different == 0) {
        first_different_index = i;
        first_different_left = bytes_[i];
        first_different_right = buffer->bytes_[i];
      }
      num_different++;
    }
  }

  if (num_different) {
    return Result{"Buffers have different values. " +
                  std::to_string(num_different) +
                  " values differed, first difference at byte " +
                  std::to_string(first_different_index) + " values " +
                  std::to_string(first_different_left) +
                  " != " + std::to_string(first_different_right)};
  }

  return {};
}

std::vector<double> Buffer::CalculateDiffs(const Buffer* buffer) const {
  std::vector<double> diffs;

  auto* buf_1_ptr = GetValues<uint8_t>();
  auto* buf_2_ptr = buffer->GetValues<uint8_t>();
  const auto& segments = format_->GetSegments();
  for (size_t i = 0; i < ElementCount(); ++i) {
    for (const auto& seg : segments) {
      if (seg.IsPadding()) {
        buf_1_ptr += seg.PaddingBytes();
        buf_2_ptr += seg.PaddingBytes();
        continue;
      }

      diffs.push_back(CalculateDiff(&seg, buf_1_ptr, buf_2_ptr));

      buf_1_ptr += seg.SizeInBytes();
      buf_2_ptr += seg.SizeInBytes();
    }
  }

  return diffs;
}

Result Buffer::CompareRMSE(Buffer* buffer, float tolerance) const {
  if (!buffer->format_->Equal(format_))
    return Result{"Buffers have a different format"};
  if (buffer->element_count_ != element_count_)
    return Result{"Buffers have a different size"};
  if (buffer->width_ != width_)
    return Result{"Buffers have a different width"};
  if (buffer->height_ != height_)
    return Result{"Buffers have a different height"};
  if (buffer->ValueCount() != ValueCount())
    return Result{"Buffers have a different number of values"};

  auto diffs = CalculateDiffs(buffer);
  double sum = 0.0;
  for (const auto val : diffs)
    sum += (val * val);

  sum /= static_cast<double>(diffs.size());
  double rmse = std::sqrt(sum);
  if (rmse > static_cast<double>(tolerance)) {
    return Result("Root Mean Square Error of " + std::to_string(rmse) +
                  " is greater than tolerance of " + std::to_string(tolerance));
  }

  return {};
}

Result Buffer::SetData(const std::vector<Value>& data) {
  return SetDataWithOffset(data, 0);
}

Result Buffer::RecalculateMaxSizeInBytes(const std::vector<Value>& data,
                                         uint32_t offset) {
  // Multiply by the input needed because the value count will use the needed
  // input as the multiplier
  uint32_t value_count =
      ((offset / format_->SizeInBytes()) * format_->InputNeededPerElement()) +
      static_cast<uint32_t>(data.size());
  uint32_t element_count = value_count;
  if (!format_->IsPacked()) {
    // This divides by the needed input values, not the values per element.
    // The assumption being the values coming in are read from the input,
    // where components are specified. The needed values maybe less then the
    // values per element.
    element_count = value_count / format_->InputNeededPerElement();
  }
  if (GetMaxSizeInBytes() < element_count * format_->SizeInBytes())
    SetMaxSizeInBytes(element_count * format_->SizeInBytes());
  return {};
}

Result Buffer::SetDataWithOffset(const std::vector<Value>& data,
                                 uint32_t offset) {
  // Multiply by the input needed because the value count will use the needed
  // input as the multiplier
  uint32_t value_count =
      ((offset / format_->SizeInBytes()) * format_->InputNeededPerElement()) +
      static_cast<uint32_t>(data.size());

  // The buffer should only be resized to become bigger. This means that if a
  // command was run to set the buffer size we'll honour that size until a
  // request happens to make the buffer bigger.
  if (value_count > ValueCount())
    SetValueCount(value_count);

  // Even if the value count doesn't change, the buffer is still resized because
  // this maybe the first time data is set into the buffer.
  bytes_.resize(GetSizeInBytes());

  // Set the new memory to zero to be on the safe side.
  uint32_t new_space =
      (static_cast<uint32_t>(data.size()) / format_->InputNeededPerElement()) *
      format_->SizeInBytes();
  assert(new_space + offset <= GetSizeInBytes());

  if (new_space > 0)
    memset(bytes_.data() + offset, 0, new_space);

  if (data.size() > (ElementCount() * format_->InputNeededPerElement()))
    return Result("Mismatched number of items in buffer");

  uint8_t* ptr = bytes_.data() + offset;
  const auto& segments = format_->GetSegments();
  for (uint32_t i = 0; i < data.size();) {
    for (const auto& seg : segments) {
      if (seg.IsPadding()) {
        ptr += seg.PaddingBytes();
        continue;
      }

      Value v = data[i++];
      ptr += WriteValueFromComponent(v, seg.GetFormatMode(), seg.GetNumBits(),
                                     ptr);
    }
  }
  return {};
}

uint32_t Buffer::WriteValueFromComponent(const Value& value,
                                         FormatMode mode,
                                         uint32_t num_bits,
                                         uint8_t* ptr) {
  if (type::Type::IsInt8(mode, num_bits)) {
    *(ValuesAs<int8_t>(ptr)) = value.AsInt8();
    return sizeof(int8_t);
  }
  if (type::Type::IsInt16(mode, num_bits)) {
    *(ValuesAs<int16_t>(ptr)) = value.AsInt16();
    return sizeof(int16_t);
  }
  if (type::Type::IsInt32(mode, num_bits)) {
    *(ValuesAs<int32_t>(ptr)) = value.AsInt32();
    return sizeof(int32_t);
  }
  if (type::Type::IsInt64(mode, num_bits)) {
    *(ValuesAs<int64_t>(ptr)) = value.AsInt64();
    return sizeof(int64_t);
  }
  if (type::Type::IsUint8(mode, num_bits)) {
    *(ValuesAs<uint8_t>(ptr)) = value.AsUint8();
    return sizeof(uint8_t);
  }
  if (type::Type::IsUint16(mode, num_bits)) {
    *(ValuesAs<uint16_t>(ptr)) = value.AsUint16();
    return sizeof(uint16_t);
  }
  if (type::Type::IsUint32(mode, num_bits)) {
    *(ValuesAs<uint32_t>(ptr)) = value.AsUint32();
    return sizeof(uint32_t);
  }
  if (type::Type::IsUint64(mode, num_bits)) {
    *(ValuesAs<uint64_t>(ptr)) = value.AsUint64();
    return sizeof(uint64_t);
  }
  if (type::Type::IsFloat16(mode, num_bits)) {
    *(ValuesAs<uint16_t>(ptr)) = FloatToHexFloat16(value.AsFloat());
    return sizeof(uint16_t);
  }
  if (type::Type::IsFloat32(mode, num_bits)) {
    *(ValuesAs<float>(ptr)) = value.AsFloat();
    return sizeof(float);
  }
  if (type::Type::IsFloat64(mode, num_bits)) {
    *(ValuesAs<double>(ptr)) = value.AsDouble();
    return sizeof(double);
  }

  // The float 10 and float 11 sizes are only used in PACKED formats.
  assert(false && "Not reached");
  return 0;
}

void Buffer::SetSizeInElements(uint32_t element_count) {
  element_count_ = element_count;
  bytes_.resize(element_count * format_->SizeInBytes());
}

void Buffer::SetSizeInBytes(uint32_t size_in_bytes) {
  assert(size_in_bytes % format_->SizeInBytes() == 0);
  element_count_ = size_in_bytes / format_->SizeInBytes();
  bytes_.resize(size_in_bytes);
}

void Buffer::SetMaxSizeInBytes(uint32_t max_size_in_bytes) {
  max_size_in_bytes_ = max_size_in_bytes;
}

uint32_t Buffer::GetMaxSizeInBytes() const {
  if (max_size_in_bytes_ != 0)
    return max_size_in_bytes_;
  else
    return GetSizeInBytes();
}

Result Buffer::SetDataFromBuffer(const Buffer* src, uint32_t offset) {
  if (bytes_.size() < offset + src->bytes_.size())
    bytes_.resize(offset + src->bytes_.size());

  std::memcpy(bytes_.data() + offset, src->bytes_.data(), src->bytes_.size());
  element_count_ =
      static_cast<uint32_t>(bytes_.size()) / format_->SizeInBytes();
  return {};
}

}  // namespace amber
