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
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or parseried.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>

#include "gtest/gtest.h"
#include "src/format.h"
#include "src/vkscript/parser.h"

namespace amber {
namespace vkscript {

using VkScriptParserTest = testing::Test;

TEST_F(VkScriptParserTest, RequireBlockNoArgumentFeatures) {
  struct {
    const char* name;
  } features[] = {{"robustBufferAccess"},
                  {"fullDrawIndexUint32"},
                  {"imageCubeArray"},
                  {"independentBlend"},
                  {"geometryShader"},
                  {"tessellationShader"},
                  {"sampleRateShading"},
                  {"dualSrcBlend"},
                  {"logicOp"},
                  {"multiDrawIndirect"},
                  {"drawIndirectFirstInstance"},
                  {"depthClamp"},
                  {"depthBiasClamp"},
                  {"fillModeNonSolid"},
                  {"depthBounds"},
                  {"wideLines"},
                  {"largePoints"},
                  {"alphaToOne"},
                  {"multiViewport"},
                  {"samplerAnisotropy"},
                  {"textureCompressionETC2"},
                  {"textureCompressionASTC_LDR"},
                  {"textureCompressionBC"},
                  {"occlusionQueryPrecise"},
                  {"pipelineStatisticsQuery"},
                  {"vertexPipelineStoresAndAtomics"},
                  {"fragmentStoresAndAtomics"},
                  {"shaderTessellationAndGeometryPointSize"},
                  {"shaderImageGatherExtended"},
                  {"shaderStorageImageExtendedFormats"},
                  {"shaderStorageImageMultisample"},
                  {"shaderStorageImageReadWithoutFormat"},
                  {"shaderStorageImageWriteWithoutFormat"},
                  {"shaderUniformBufferArrayDynamicIndexing"},
                  {"shaderSampledImageArrayDynamicIndexing"},
                  {"shaderStorageBufferArrayDynamicIndexing"},
                  {"shaderStorageImageArrayDynamicIndexing"},
                  {"shaderClipDistance"},
                  {"shaderCullDistance"},
                  {"shaderFloat64"},
                  {"shaderInt64"},
                  {"shaderInt16"},
                  {"shaderResourceResidency"},
                  {"shaderResourceMinLod"},
                  {"sparseBinding"},
                  {"sparseResidencyBuffer"},
                  {"sparseResidencyImage2D"},
                  {"sparseResidencyImage3D"},
                  {"sparseResidency2Samples"},
                  {"sparseResidency4Samples"},
                  {"sparseResidency8Samples"},
                  {"sparseResidency16Samples"},
                  {"sparseResidencyAliased"},
                  {"variableMultisampleRate"},
                  {"inheritedQueries"},
                  {"VariablePointerFeatures.variablePointers"},
                  {"VariablePointerFeatures.variablePointersStorageBuffer"}};

  for (const auto& feature : features) {
    std::string in = std::string("[require]\n") + feature.name + "\n";

    Parser parser;
    Result r = parser.Parse(in);
    ASSERT_TRUE(r.IsSuccess()) << r.Error();

    auto script = parser.GetScript();
    auto feats = script->GetRequiredFeatures();
    ASSERT_EQ(1U, feats.size());
    EXPECT_EQ(feature.name, feats[0]);
  }
}

TEST_F(VkScriptParserTest, RequireBlockExtensions) {
  std::string block = R"([require]
VK_KHR_storage_buffer_storage_class
VK_KHR_variable_pointers
VK_KHR_get_physical_device_properties2)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  auto device_exts = script->GetRequiredDeviceExtensions();
  ASSERT_EQ(2U, device_exts.size());
  EXPECT_EQ("VK_KHR_storage_buffer_storage_class", device_exts[0]);
  EXPECT_EQ("VK_KHR_variable_pointers", device_exts[1]);

  auto inst_exts = script->GetRequiredInstanceExtensions();
  ASSERT_EQ(1U, inst_exts.size());
  EXPECT_EQ("VK_KHR_get_physical_device_properties2", inst_exts[0]);
}

TEST_F(VkScriptParserTest, RequireBlockFramebuffer) {
  std::string block = "[require]\nframebuffer R32G32B32A32_SFLOAT";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess());

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(1U, buffers.size());
  EXPECT_EQ(BufferType::kColor, buffers[0]->GetBufferType());
  EXPECT_TRUE(buffers[0]->IsFormatBuffer());
  EXPECT_EQ(FormatType::kR32G32B32A32_SFLOAT,
            buffers[0]->AsFormatBuffer()->GetFormat().GetFormatType());
}

TEST_F(VkScriptParserTest, RequireBlockDepthStencil) {
  std::string block = "[require]\ndepthstencil D24_UNORM_S8_UINT";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(2U, buffers.size());
  EXPECT_EQ(BufferType::kDepth, buffers[1]->GetBufferType());
  EXPECT_TRUE(buffers[1]->IsFormatBuffer());
  EXPECT_EQ(FormatType::kD24_UNORM_S8_UINT,
            buffers[1]->AsFormatBuffer()->GetFormat().GetFormatType());
}

TEST_F(VkScriptParserTest, RequireBlockMultipleLines) {
  std::string block = R"([require]
# Requirements block stuff.
depthstencil D24_UNORM_S8_UINT
sparseResidency4Samples
framebuffer R32G32B32A32_SFLOAT
# More comments
inheritedQueries # line comment
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(2U, buffers.size());
  EXPECT_EQ(BufferType::kColor, buffers[0]->GetBufferType());
  EXPECT_TRUE(buffers[0]->IsFormatBuffer());
  EXPECT_EQ(FormatType::kR32G32B32A32_SFLOAT,
            buffers[0]->AsFormatBuffer()->GetFormat().GetFormatType());

  EXPECT_EQ(BufferType::kDepth, buffers[1]->GetBufferType());
  EXPECT_TRUE(buffers[1]->IsFormatBuffer());
  EXPECT_EQ(FormatType::kD24_UNORM_S8_UINT,
            buffers[1]->AsFormatBuffer()->GetFormat().GetFormatType());

  auto feats = script->GetRequiredFeatures();
  EXPECT_EQ("sparseResidency4Samples", feats[0]);
  EXPECT_EQ("inheritedQueries", feats[1]);
}

TEST_F(VkScriptParserTest, IndicesBlock) {
  std::string block = "[indices]\n1 2 3";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(2U, buffers.size());
  ASSERT_EQ(BufferType::kIndex, buffers[1]->GetBufferType());

  auto buffer_ptr = buffers[1].get();
  ASSERT_TRUE(buffer_ptr->IsDataBuffer());

  auto buffer = buffer_ptr->AsDataBuffer();
  EXPECT_TRUE(buffer->GetDatumType().IsUint16());
  EXPECT_EQ(3U, buffer->GetSize());
  auto& data = buffer->GetData();
  ASSERT_EQ(3U, data.size());

  EXPECT_TRUE(data[0].IsInteger());
  EXPECT_EQ(1, data[0].AsUint16());
  EXPECT_TRUE(data[1].IsInteger());
  EXPECT_EQ(2, data[1].AsUint16());
  EXPECT_TRUE(data[2].IsInteger());
  EXPECT_EQ(3, data[2].AsUint16());
}

TEST_F(VkScriptParserTest, IndicesBlockMultipleLines) {
  std::string block = R"([indices]
# comment line
1 2 3   4 5 6
# another comment
7 8 9  10 11 12
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  auto& buffers = script->GetBuffers();
  ASSERT_EQ(2U, buffers.size());
  ASSERT_EQ(buffers[1]->GetBufferType(), BufferType::kIndex);

  auto& data = buffers[1]->GetData();
  std::vector<uint16_t> results = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  ASSERT_EQ(results.size(), data.size());
  for (size_t i = 0; i < results.size(); ++i) {
    EXPECT_TRUE(data[i].IsInteger());
    EXPECT_EQ(results[i], data[i].AsUint16());
  }
}

TEST_F(VkScriptParserTest, IndicesBlockBadValue) {
  std::string block = "[indices]\n1 a 3";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("1: Invalid value in indices block: a", r.Error());
}

TEST_F(VkScriptParserTest, IndicesBlockValueTooLarge) {
  std::string block = "[indices]\n100000000000 3";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("1: Value too large in indices block: 100000000000", r.Error());
}

TEST_F(VkScriptParserTest, VertexDataEmpty) {
  std::string block = "[vertex data]\n#comment\n";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess());

  auto script = parser.GetScript();
  EXPECT_EQ(1U, script->GetBuffers().size());
}

TEST_F(VkScriptParserTest, VertexDataHeaderFormatString) {
  std::string block = "[vertex data]\n0/R32G32_SFLOAT 1/A8B8G8R8_UNORM_PACK32";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(3U, buffers.size());

  ASSERT_EQ(BufferType::kVertex, buffers[1]->GetBufferType());
  EXPECT_EQ(static_cast<uint8_t>(0U), buffers[1]->GetLocation());
  EXPECT_EQ(FormatType::kR32G32_SFLOAT,
            buffers[1]->AsFormatBuffer()->GetFormat().GetFormatType());
  EXPECT_TRUE(buffers[1]->GetData().empty());

  ASSERT_EQ(BufferType::kVertex, buffers[2]->GetBufferType());
  EXPECT_EQ(1U, buffers[2]->GetLocation());
  EXPECT_EQ(FormatType::kA8B8G8R8_UNORM_PACK32,
            buffers[2]->AsFormatBuffer()->GetFormat().GetFormatType());
  EXPECT_TRUE(buffers[2]->GetData().empty());
}

TEST_F(VkScriptParserTest, VertexDataHeaderGlslString) {
  std::string block = "[vertex data]\n0/float/vec2 1/int/vec3";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(3U, buffers.size());

  ASSERT_EQ(BufferType::kVertex, buffers[1]->GetBufferType());
  EXPECT_EQ(static_cast<uint8_t>(0U), buffers[1]->GetLocation());
  EXPECT_EQ(FormatType::kR32G32_SFLOAT,
            buffers[1]->AsFormatBuffer()->GetFormat().GetFormatType());
  auto& comps1 = buffers[1]->AsFormatBuffer()->GetFormat().GetComponents();
  ASSERT_EQ(2U, comps1.size());
  EXPECT_EQ(FormatMode::kSFloat, comps1[0].mode);
  EXPECT_EQ(FormatMode::kSFloat, comps1[1].mode);
  EXPECT_TRUE(buffers[1]->GetData().empty());

  ASSERT_EQ(BufferType::kVertex, buffers[2]->GetBufferType());
  EXPECT_EQ(1U, buffers[2]->GetLocation());
  EXPECT_EQ(FormatType::kR32G32B32_SINT,
            buffers[2]->AsFormatBuffer()->GetFormat().GetFormatType());
  auto& comps2 = buffers[2]->AsFormatBuffer()->GetFormat().GetComponents();
  ASSERT_EQ(3U, comps2.size());
  EXPECT_EQ(FormatMode::kSInt, comps2[0].mode);
  EXPECT_EQ(FormatMode::kSInt, comps2[1].mode);
  EXPECT_EQ(FormatMode::kSInt, comps2[2].mode);
  EXPECT_TRUE(buffers[2]->GetData().empty());
}

TEST_F(VkScriptParserTest, TestBlock) {
  std::string block = R"([test]
clear color 255 255 255 0
clear depth 10
clear stencil 2
clear)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& cmds = script->GetCommands();
  ASSERT_EQ(4U, cmds.size());

  ASSERT_TRUE(cmds[0]->IsClearColor());
  auto* color_cmd = cmds[0]->AsClearColor();
  EXPECT_FLOAT_EQ(255.f, color_cmd->GetR());
  EXPECT_FLOAT_EQ(255.f, color_cmd->GetG());
  EXPECT_FLOAT_EQ(255.f, color_cmd->GetB());
  EXPECT_FLOAT_EQ(0.0f, color_cmd->GetA());

  ASSERT_TRUE(cmds[1]->IsClearDepth());
  EXPECT_EQ(10U, cmds[1]->AsClearDepth()->GetValue());

  ASSERT_TRUE(cmds[2]->IsClearStencil());
  EXPECT_EQ(2U, cmds[2]->AsClearStencil()->GetValue());

  EXPECT_TRUE(cmds[3]->IsClear());
}

TEST_F(VkScriptParserTest, VertexDataRows) {
  std::string block = R"([vertex data]
# Vertex data
0/R32G32B32_SFLOAT  1/R8G8B8_UNORM
-1    -1 0.25       255 0 0  # ending comment
# Another Row
0.25  -1 0.25       255 0 255
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(3U, buffers.size());

  ASSERT_EQ(BufferType::kVertex, buffers[1]->GetBufferType());

  std::vector<float> seg_0 = {-1.f, -1.f, 0.25f, 0.25f, -1.f, 0.25f};
  const auto& values_0 = buffers[1]->GetData();
  ASSERT_EQ(seg_0.size(), values_0.size());
  for (size_t i = 0; i < seg_0.size(); ++i) {
    ASSERT_TRUE(values_0[i].IsFloat());
    EXPECT_FLOAT_EQ(seg_0[i], values_0[i].AsFloat());
  }

  ASSERT_EQ(BufferType::kVertex, buffers[2]->GetBufferType());

  std::vector<uint8_t> seg_1 = {255, 0, 0, 255, 0, 255};
  const auto& values_1 = buffers[2]->GetData();
  ASSERT_EQ(seg_1.size(), values_1.size());
  for (size_t i = 0; i < seg_1.size(); ++i) {
    ASSERT_TRUE(values_1[i].IsInteger());
    EXPECT_EQ(seg_1[i], values_1[i].AsUint8());
  }
}

TEST_F(VkScriptParserTest, VertexDataShortRow) {
  std::string block = R"([vertex data]
0/R32G32B32_SFLOAT  1/R8G8B8_UNORM
-1    -1 0.25       255 0 0
0.25  -1 0.25       255 0
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("3: Too few cells in given vertex data row", r.Error());
}

TEST_F(VkScriptParserTest, VertexDataIncorrectValue) {
  std::string block = R"([vertex data]
0/R32G32B32_SFLOAT  1/R8G8B8_UNORM
-1    -1 0.25       255 StringValue 0
0.25  -1 0.25       255 0 0
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("2: Invalid vertex data value: StringValue", r.Error());
}

TEST_F(VkScriptParserTest, VertexDataRowsWithHex) {
  std::string block = R"([vertex data]
0/A8B8G8R8_UNORM_PACK32
0xff0000ff
0xffff0000
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_TRUE(r.IsSuccess()) << r.Error();

  auto script = parser.GetScript();
  const auto& buffers = script->GetBuffers();
  ASSERT_EQ(2U, buffers.size());
  ASSERT_EQ(BufferType::kVertex, buffers[1]->GetBufferType());

  std::vector<uint32_t> seg_0 = {0xff0000ff, 0xffff0000};
  const auto& values_0 = buffers[1]->GetData();
  ASSERT_EQ(seg_0.size(), values_0.size());

  for (size_t i = 0; i < seg_0.size(); ++i) {
    ASSERT_TRUE(values_0[i].IsInteger());
    EXPECT_EQ(seg_0[i], values_0[i].AsUint32());
  }
}

TEST_F(VkScriptParserTest, VertexDataRowsWithHexWrongColumn) {
  std::string block = R"([vertex data]
0/R32G32B32_SFLOAT  1/R8G8B8_UNORM
-1    -1 0.25       0xffff0000
0.25  -1 0.25       255 0
)";

  Parser parser;
  Result r = parser.Parse(block);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("2: Invalid vertex data value: 0xffff0000", r.Error());
}

TEST_F(VkScriptParserTest, ErrorLineNumberBug195) {
  std::string input = R"([compute shader]
#version 430

void main() {
}

[test]
# Error must report "9: Unknown command: unknown"
unknown
})";

  Parser parser;
  Result r = parser.Parse(input);
  ASSERT_FALSE(r.IsSuccess());
  EXPECT_EQ("9: Unknown command: unknown", r.Error());
}

}  // namespace vkscript
}  // namespace amber
