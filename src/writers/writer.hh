#ifndef H_ACQUIRE_ZARR_WRITER_V0
#define H_ACQUIRE_ZARR_WRITER_V0

#ifndef __cplusplus
#error "This header requires C++20"
#endif

#include "platform.h"
#include "device/props/components.h"

#include "../common.hh"
#include "blosc.compressor.hh"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace fs = std::filesystem;

namespace acquire::sink::zarr {
struct Zarr;

struct FileCreator
{
    FileCreator() = delete;
    explicit FileCreator(Zarr* zarr);
    ~FileCreator() noexcept = default;

    void set_base_dir(const fs::path& base_dir) noexcept;
    [[nodiscard]] bool create(int n_c,
                              int n_y,
                              int n_x,
                              std::vector<file>& files) noexcept;

  private:
    fs::path base_dir_;
    Zarr* zarr_;

    bool create_channel_dirs_(int n_c) noexcept;
};

struct Writer
{
  public:
    Writer() = delete;
    Writer(const ImageDims& frame_dims,
           const ImageDims& tile_dims,
           uint32_t frames_per_chunk,
           const std::string& data_root,
           Zarr* zarr);

    /// Constructor with Blosc compression params
    Writer(const ImageDims& frame_dims,
           const ImageDims& tile_dims,
           uint32_t frames_per_chunk,
           const std::string& data_root,
           Zarr* zarr,
           const BloscCompressionParams& compression_params);
    virtual ~Writer();

    [[nodiscard]] virtual bool write(const VideoFrame* frame) noexcept = 0;
    void finalize() noexcept;

    uint32_t frames_written() const noexcept;

  protected:
    /// Tiling/chunking
    ImageDims frame_dims_;
    ImageDims tile_dims_;
    uint16_t tiles_per_frame_x_;
    uint16_t tiles_per_frame_y_;
    SampleType pixel_type_;
    uint32_t frames_per_chunk_;

    /// Compression
    std::optional<BloscCompressionParams> blosc_compression_params_;

    /// Filesystem
    FileCreator file_creator_;
    fs::path data_root_;
    std::vector<file> files_;

    /// Multithreading
    std::vector<std::vector<uint8_t>> chunk_buffers_;
    bool* buffers_ready_;
    std::mutex buffers_mutex_;

    /// Bookkeeping
    uint64_t bytes_to_flush_;
    uint32_t frames_written_;
    uint32_t current_chunk_;
    Zarr* zarr_;

    [[nodiscard]] bool validate_frame_(const VideoFrame* frame) noexcept;

    virtual void make_buffers_() noexcept = 0;

    void finalize_chunks_() noexcept;
    std::vector<size_t> compress_buffers_() noexcept;
    virtual size_t write_bytes_(const uint8_t* buf,
                                size_t buf_size) noexcept = 0;
    virtual void flush_() noexcept = 0;

    uint32_t tiles_per_frame_() const;

    /// Files
    [[nodiscard]] virtual bool make_files_() noexcept = 0;
    void close_files_();
    void rollover_();
};
} // namespace acquire::sink::zarr

#endif // H_ACQUIRE_ZARR_WRITER_V0