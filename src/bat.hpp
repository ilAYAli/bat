#ifndef BAT_HPP
#define BAT_HPP

#include <sys/types.h>

#include "boost/optional.hpp"
#include <stdint.h>
#include <string>
#include <vector>

const std::string COLOR_NORMAL  = "\33[0m";
const std::string COLOR_LIGHT   = "\33[2m";
const std::string COLOR_BOLD    = "\33[1m";
const std::string COLOR_BLACK   = "\33[01;30m";
const std::string COLOR_RED     = "\33[31m";
const std::string COLOR_GREEN   = "\33[01;32m";
const std::string COLOR_YELLOW  = "\33[01;33m";
const std::string COLOR_BLUE    = "\33[01;34m";
const std::string COLOR_MAGENTA = "\33[01;35m";
const std::string COLOR_CYAN    = "\33[01;36m";
const std::string COLOR_WHITE   = "\33[01;37m";

enum option_flags {
    PRINT_COLORS = 1,
    SWAP_ENDIAN  = 2,
    PRINT_OFFSET = 16,
    PRINT_HEX    = 32,
    PRINT_BIN    = 64,
    PRINT_WORDS  = 128,
    PRINT_ASCII  = 256,
    PRINT_ARRAY  = 512
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
        , print_flags(PRINT_OFFSET | PRINT_ASCII | PRINT_HEX)
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
    unsigned print_flags;
    mutable bool colorize;
};

boost::optional<config> parse_args(int argc, char ** argv);

class bat {
public:
    bat(config & cfg)
        : cfg(cfg)
        , fp_(stdin)
        , fpo_(stdout)
        , source_file_size()
        , dest_file_size()
        , quantum_(8192)
    { }

    virtual ~bat();

    bool parse(const std::string & source_file,
               const std::string & dest_file,
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
    FILE * fp_;
    FILE * fpo_;
    std::size_t source_file_size;
    std::size_t dest_file_size;
    std::vector<char> quantum_;
};

#endif
