#ifndef BAT_HPP
#define BAT_HPP

#include "boost/optional.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <iostream>
#include <vector>

namespace pwa {

enum class opt {
    swap_endian  = 1,
    print_colors = 2,
    print_offset = 16,
    print_hex    = 32,
    print_bin    = 64,
    print_words  = 128,
    print_ascii  = 256,
    print_array  = 512
};

opt operator | (opt lhs, opt rhs)
{
    return static_cast<opt>(
        static_cast<std::underlying_type<opt>::type>(lhs) |
        static_cast<std::underlying_type<opt>::type>(rhs));
}

unsigned operator & (opt lhs, opt rhs)
{
    return static_cast<unsigned>(
        static_cast<std::underlying_type<opt>::type>(lhs) &
        static_cast<std::underlying_type<opt>::type>(rhs));
}

opt & operator |= (opt & lhs, opt rhs)
{
    lhs = static_cast<opt> (
        static_cast<std::underlying_type<opt>::type>(lhs) |
        static_cast<std::underlying_type<opt>::type>(rhs)
    );

    return lhs;
}

opt & operator &= (opt & lhs, opt rhs)
{
    lhs = static_cast<opt> (
        static_cast<std::underlying_type<opt>::type>(lhs) &
        static_cast<std::underlying_type<opt>::type>(rhs)
    );

    return lhs;
}

opt operator ~ (opt rhs)
{
    return static_cast<opt> (
        ~static_cast<std::underlying_type<opt>::type>(rhs)
    );
}

std::ifstream::pos_type filesize(const std::string & filename)
{
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    return in.tellg();
}

class File {
public:
    bool open(std::string path, std::string mode, std::size_t offset = 0) {
        name_ = std::move(path);
        mode_ = std::move(mode);

        if ((mode_.at(0) == 'r') && name_.empty()) {
            fp_ = stdin;
        } else if ((mode_.at(0) == 'w') && (name_.empty() || (name_ == "-"))) {
            fp_ = stdout;
        } else {
            fp_ = fopen(name_.c_str(), mode_.c_str());
            if (!fp_) {
                std::cerr << "error, could not open file: '" + name_ << "'" << std::endl;
                return false;
            }

            size_ = filesize(name_);
            offset_ = fseek(fp_, offset, SEEK_SET);
        }
        return true;
    }

    File(std::string path, std::string mode, std::size_t offset = 0) {
        if (!open(std::move(path), std::move(mode), offset)) {
            throw std::runtime_error("error, could not open file");
        }
    }

    File() = default;
    ~File() {
        if (fp_ && (fp_ != stdin) && (fp_ != stdout))
            fclose(fp_);
        fflush(fp_);
    }
    File(const File & ) = delete;               // copy
    File & operator= (const File & ) = delete;  // copy assign
    File (File &&) = delete;                    // move
    File operator=(File &&) = delete;           // move assign
    FILE * operator ()() const { return fp_; }
    std::size_t size() const { return size_; }
private:
    std::string name_;
    std::string mode_;
    std::size_t size_;
    std::size_t offset_;
    FILE * fp_;
};


class config {
public:
    config()
        : source_offset()
        , dest_offset()
        , bytes_to_read(SIZE_MAX)
        , array_mode(false)
        , hex_mode(true)
        , bin_mode(false)
        , verbose(false)
        , relative_offset()
        , bytes_per_line(16)
        , bytes_on_line()
        , source_file()
        , dest_file()
        , print_flags(opt::print_offset | opt::print_ascii | opt::print_hex)
        , colorize()
    { }
    off_t source_offset;
    off_t dest_offset;
    std::size_t bytes_to_read;
    bool array_mode;
    bool hex_mode;
    bool bin_mode;
    bool verbose;
    std::size_t relative_offset;
    std::size_t bytes_per_line;
    std::size_t bytes_on_line;
    std::string source_file;
    std::string dest_file;
    opt print_flags;
    mutable bool colorize;
};

class bat {
public:
    bat(config & cfg)
        : cfg(cfg)
        , src_()
        , dst_()
        , source_file_size()
        , dest_file_size()
        , quantum_(8192)
    { }

    bool parse(std::string source_file,
               std::string dest_file,
               const std::size_t source_offset,
               const std::size_t dest_offset);
private:
    void print_colorized(const std::string & col) const;
    void print_array() const;
    void print_hex() const;
    void print_binary() const;
    void print_words() const;
    void print_ascii() const;
    void formated_output();

    config cfg;
    File src_;
    File dst_;
    std::size_t source_file_size;
    std::size_t dest_file_size;
    std::vector<unsigned char> quantum_;
};

} // end namespace pwa

#endif
