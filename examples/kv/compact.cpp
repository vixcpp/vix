/**
 *
 *  @file compact.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/kv
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix KV
 *
 *  Manual compaction example
 *
 */

#include <vix/kv/compaction/Compactor.hpp>
#include <vix/kv/core/KvConfig.hpp>
#include <vix/kv/records/KvRecord.hpp>
#include <vix/kv/storage/FileLayout.hpp>
#include <vix/kv/storage/Segment.hpp>
#include <vix/kv/storage/SegmentReader.hpp>
#include <vix/kv/storage/SegmentWriter.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
  namespace compaction = vix::kv::compaction;
  namespace core = vix::kv::core;
  namespace records = vix::kv::records;
  namespace storage = vix::kv::storage;

  std::filesystem::path data_path()
  {
    return std::filesystem::path{"examples_data"} / "compact";
  }

  int fail(const std::string &message)
  {
    std::cerr << message << '\n';
    return 1;
  }

  std::vector<std::uint8_t> bytes(std::string text)
  {
    return std::vector<std::uint8_t>(
        text.begin(),
        text.end());
  }

  records::KvRecord put_record(
      std::uint64_t sequence,
      std::string key,
      std::string value)
  {
    return records::KvRecord::put(
        std::move(key),
        bytes(std::move(value)),
        sequence,
        sequence * 100);
  }

  records::KvRecord delete_record(
      std::uint64_t sequence,
      std::string key)
  {
    return records::KvRecord::remove(
        std::move(key),
        sequence,
        sequence * 100);
  }

  core::KvResult<storage::Segment> write_segment(
      const core::KvConfig &config,
      std::uint64_t segment_id,
      const std::vector<records::KvRecord> &items)
  {
    storage::SegmentWriter writer{config, segment_id};

    auto opened = writer.open(true);

    if (opened.is_err())
    {
      return core::KvResult<storage::Segment>::err(
          opened.error());
    }

    for (const auto &record : items)
    {
      auto written = writer.append(record);

      if (written.is_err())
      {
        (void)writer.close();

        return core::KvResult<storage::Segment>::err(
            written.error());
      }
    }

    auto closed = writer.close();

    if (closed.is_err())
    {
      return core::KvResult<storage::Segment>::err(
          closed.error());
    }

    return core::KvResult<storage::Segment>::ok(
        writer.segment());
  }

  core::KvResult<std::vector<records::KvRecord>>
  read_segment_records(const storage::Segment &segment)
  {
    storage::SegmentReader reader{segment};

    auto opened = reader.open();

    if (opened.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          opened.error());
    }

    auto records_read = reader.read_all();

    auto closed = reader.close();

    if (closed.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          closed.error());
    }

    if (records_read.is_err())
    {
      return core::KvResult<std::vector<records::KvRecord>>::err(
          records_read.error());
    }

    return records_read;
  }

  void print_record(const records::KvRecord &record)
  {
    std::cout << "key      : " << record.key << '\n';
    std::cout << "sequence : " << record.header.sequence << '\n';
    std::cout << "type     : "
              << records::to_string(record.header.type)
              << '\n';

    if (record.has_value())
    {
      const std::string value(
          record.value.begin(),
          record.value.end());

      std::cout << "value    : " << value << '\n';
    }
    else
    {
      std::cout << "value    : <none>\n";
    }

    std::cout << '\n';
  }

  int print_segment(
      const std::string &title,
      const storage::Segment &segment)
  {
    auto records_read = read_segment_records(segment);

    if (records_read.is_err())
    {
      return fail(
          "failed to read segment " +
          std::to_string(segment.id) +
          ": " +
          records_read.error().message());
    }

    std::cout << title << '\n';
    std::cout << std::string(title.size(), '-') << '\n';

    std::cout << "segment id   : "
              << segment.id
              << '\n';

    std::cout << "record count : "
              << records_read.value().size()
              << "\n\n";

    if (records_read.value().empty())
    {
      std::cout << "no records\n\n";
      return 0;
    }

    for (const auto &record : records_read.value())
    {
      print_record(record);
    }

    return 0;
  }

  int run_compaction_example()
  {
    const auto root = data_path();

    std::filesystem::remove_all(root);

    auto config = core::KvConfig::durable(root);

    std::vector<records::KvRecord> first_records;
    first_records.push_back(
        put_record(1, "v1|5:users1:1", "Ada"));
    first_records.push_back(
        put_record(2, "v1|5:users1:2", "Grace"));

    auto first_segment = write_segment(
        config,
        1,
        first_records);

    if (first_segment.is_err())
    {
      return fail(
          "failed to write first segment: " +
          first_segment.error().message());
    }

    std::vector<records::KvRecord> second_records;
    second_records.push_back(
        put_record(3, "v1|5:users1:1", "Ada Lovelace"));
    second_records.push_back(
        delete_record(4, "v1|5:users1:2"));
    second_records.push_back(
        put_record(5, "v1|5:users1:3", "Linus"));

    auto second_segment = write_segment(
        config,
        2,
        second_records);

    if (second_segment.is_err())
    {
      return fail(
          "failed to write second segment: " +
          second_segment.error().message());
    }

    auto segment_1 = first_segment.move_value();
    auto segment_2 = second_segment.move_value();

    const int first_print = print_segment(
        "input segment 1",
        segment_1);

    if (first_print != 0)
    {
      return first_print;
    }

    const int second_print = print_segment(
        "input segment 2",
        segment_2);

    if (second_print != 0)
    {
      return second_print;
    }

    const auto output_path =
        storage::FileLayout::segment_path(config, 3);

    std::vector<storage::Segment> input_segments;
    input_segments.push_back(segment_1);
    input_segments.push_back(segment_2);

    auto plan = compaction::Compactor::make_manual_plan(
        input_segments,
        3,
        output_path);

    auto validation = compaction::Compactor::validate_plan(plan);

    if (validation.is_err())
    {
      return fail(
          "invalid compaction plan: " +
          validation.error().message());
    }

    compaction::Compactor compactor;

    auto compacted = compactor.compact(plan);

    if (compacted.is_err())
    {
      return fail(
          "compaction failed: " +
          compacted.error().message());
    }

    const auto &result = compacted.value();

    std::cout << "compaction result\n";
    std::cout << "-----------------\n";
    std::cout << "success              : "
              << (result.success ? "yes" : "no")
              << '\n';

    std::cout << "input segments       : "
              << result.input_segment_count
              << '\n';

    std::cout << "input records        : "
              << result.input_record_count
              << '\n';

    std::cout << "output records       : "
              << result.output_record_count
              << '\n';

    std::cout << "obsolete records     : "
              << result.skipped_obsolete_records
              << '\n';

    std::cout << "input bytes          : "
              << result.input_bytes
              << '\n';

    std::cout << "output bytes         : "
              << result.output_bytes
              << '\n';

    std::cout << "bytes reclaimed      : "
              << result.bytes_reclaimed()
              << '\n';

    std::cout << "last sequence        : "
              << result.last_sequence
              << "\n\n";

    const int output_print = print_segment(
        "output compacted segment",
        result.output_segment);

    if (output_print != 0)
    {
      return output_print;
    }

    return 0;
  }
}

int main()
{
  const int result = run_compaction_example();

  if (result != 0)
  {
    return result;
  }

  std::cout << "compact example completed\n";
  return 0;
}
