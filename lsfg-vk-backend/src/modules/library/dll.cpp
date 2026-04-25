/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "dll.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    /// DOS file header
    struct DOSHeader {
        uint16_t magic; // 0x5A4D
        std::array<uint16_t, 29> pad;
        int32_t pe_offset; // File offset
    };

    /// PE header
    struct PEHeader {
        uint32_t signature; // "PE\0\0"
        std::array<uint16_t, 1> pad1;
        uint16_t sect_count;
        std::array<uint16_t, 6> pad2;
        uint16_t opt_hdr_size;
        std::array<uint16_t, 1> pad3;
    };

    /// (partial!) PE optional header
    struct PEOptionalHeader {
        uint16_t magic; // 0x20B
        std::array<uint16_t, 63> pad4;
        std::pair<uint32_t, uint32_t> resource_table; // File offset/size
    };

    /// Section header
    struct SectionHeader {
        std::array<uint16_t, 4> pad1;
        uint32_t vsize; // Virtual
        uint32_t vaddress;
        uint32_t fsize; // Raw
        uint32_t foffset;
        std::array<uint16_t, 8> pad2;
    };

    /// Resource directory
    struct ResourceDirectory {
        std::array<uint16_t, 6> pad;
        uint16_t name_count;
        uint16_t id_count;
    };

    /// Resource directory entry
    struct ResourceDirectoryEntry {
        uint32_t id;
        uint32_t offset; // High bit = Directory
    };

    /// Resource data entry
    struct ResourceDataEntry {
        uint32_t offset;
        uint32_t size;
        std::array<uint32_t, 2> pad;
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
    /// Safely cast a vector to a pointer of type T
    template<typename T>
    const T* safe_cast(const std::vector<uint8_t>& data, size_t offset) {
        const size_t end{offset + sizeof(T)};
        if (end > data.size() || end < offset)
            throw std::runtime_error("Buffer overflow/underflow during safe cast");
        return reinterpret_cast<const T*>(&data.at(offset)); // NOLINT (unsafe cast)
    }

    /// Safely cast a vector to a span of T
    template<typename T>
    std::span<const T> span_cast(const std::vector<uint8_t>& data, size_t offset, size_t count) {
        const size_t end{offset + (count * sizeof(T))};
        if (end > data.size() || end < offset)
            throw std::runtime_error("Buffer overflow/underflow during safe cast");
        return{ reinterpret_cast<const T*>(&data.at(offset)), count }; // NOLINT (unsafe cast)
    }
#pragma clang diagnostic pop
}

using namespace lsfgvk::library;

std::unordered_map<uint32_t, std::vector<uint32_t>> priv::parseDll(
    const std::filesystem::path& dll
) {
    std::ifstream file(dll, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("Unable to open file");

    const std::streamsize size{static_cast<std::streamsize>(file.tellg())};
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) // NOLINT (unsafe cast)
        throw std::runtime_error("Unable to read file");

    // Parse dos header
    size_t fileOffset{0};
    const auto* dosHdr{safe_cast<const DOSHeader>(data, 0)};
    if (dosHdr->magic != 0x5A4D)
        throw std::runtime_error("Magic number in DOS header is incorrect");

    // Parse pe header
    fileOffset += static_cast<size_t>(dosHdr->pe_offset);
    const auto* peHdr{safe_cast<const PEHeader>(data, fileOffset)};
    if (peHdr->signature != 0x00004550)
        throw std::runtime_error("Signature in PE header is incorrect");

    // Parse optional pe header
    fileOffset += sizeof(PEHeader);
    const auto* peOptHdr{safe_cast<const PEOptionalHeader>(data, fileOffset)};
    if (peOptHdr->magic != 0x20B)
        throw std::runtime_error("PE format is not PE32+");
    const auto& [rsrc_rva, rsrc_size] = peOptHdr->resource_table;

    // }Locate section containing resources
    std::optional<size_t> rsrc_offset;
    fileOffset += peHdr->opt_hdr_size;
    const auto sectHdrs{span_cast<const SectionHeader>(data, fileOffset, peHdr->sect_count)};
    for (const auto& sectHdr : sectHdrs) {
        if (rsrc_rva < sectHdr.vaddress || rsrc_rva > (sectHdr.vaddress + sectHdr.vsize))
            continue;

        rsrc_offset.emplace((rsrc_rva - sectHdr.vaddress) + sectHdr.foffset);
        break;
    }
    if (!rsrc_offset)
        throw std::runtime_error("Unable to locate resource section");

    // Parse resource directory
    fileOffset = rsrc_offset.value();
    const auto* rsrcDir{safe_cast<const ResourceDirectory>(data, fileOffset)};
    if (rsrcDir->id_count < 3)
        throw std::runtime_error("Resource directory does not have enough entries");

    // Find resource table with data type
    std::optional<size_t> rsrc_tbl_offset;
    fileOffset = rsrc_offset.value() + sizeof(ResourceDirectory);
    const auto rsrcDirEntries{span_cast<const ResourceDirectoryEntry>(
        data, fileOffset, rsrcDir->name_count + rsrcDir->id_count)};
    for (const auto& rsrcDirEntry : rsrcDirEntries) {
        if (rsrcDirEntry.id != 10) // RT_RCDATA
            continue;
        if ((rsrcDirEntry.offset & 0x80000000) == 0)
            throw std::runtime_error("Expected resource directory, found data entry");

        rsrc_tbl_offset.emplace(rsrcDirEntry.offset & 0x7FFFFFFF);
    }
    if (!rsrc_tbl_offset)
        throw std::runtime_error("Unable to locate RT_RCDATA directory");

    // Parse data type resource directory
    fileOffset = rsrc_offset.value() + rsrc_tbl_offset.value();
    const auto* rsrcTbl{safe_cast<const ResourceDirectory>(data, fileOffset)};
    if (rsrcTbl->id_count < 1)
        throw std::runtime_error("RT_RCDATA directory does not have enough entries");

    // Collect all resources
    fileOffset += sizeof(ResourceDirectory);
    const auto rsrcTblEntries{span_cast<const ResourceDirectoryEntry>(
        data, fileOffset, rsrcTbl->name_count + rsrcTbl->id_count)};

    std::unordered_map<uint32_t, std::vector<uint32_t>> resources;
    resources.reserve(rsrcTbl->id_count);

    for (const auto& rsrcTblEntry : rsrcTblEntries) {
        if ((rsrcTblEntry.offset & 0x80000000) == 0)
            throw std::runtime_error("Expected resource directory, found data entry");

        // Skip over language directory
        fileOffset = rsrc_offset.value() + (rsrcTblEntry.offset & 0x7FFFFFFF);
        const auto* langDir = safe_cast<const ResourceDirectory>(data, fileOffset);
        if (langDir->id_count < 1)
            throw std::runtime_error("Malformed language directory");

        fileOffset += sizeof(ResourceDirectory);
        const auto* langDirEntry{safe_cast<const ResourceDirectoryEntry>(data, fileOffset)};
        if ((langDirEntry->offset & 0x80000000) != 0)
            throw std::runtime_error("Expected resource data entry, found directory");

        // Parse resource data entry
        fileOffset = rsrc_offset.value() + (langDirEntry->offset & 0x7FFFFFFF);
        const auto* entry{safe_cast<const ResourceDataEntry>(data, fileOffset)};
        if (entry->offset < rsrc_rva || entry->offset > (rsrc_rva + rsrc_size))
            throw std::runtime_error("Resource data entry points outside resource section");

        // Extract resource
        fileOffset = (entry->offset - rsrc_rva) + rsrc_offset.value();
        const auto rdata{span_cast<const uint32_t>(
            data, fileOffset, entry->size / sizeof(uint32_t))};

        resources.emplace(rsrcTblEntry.id, std::vector<uint32_t>(rdata.begin(), rdata.end()));
    }

    return resources;
}
