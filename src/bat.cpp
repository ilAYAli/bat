// bat -- combined cat, dd and hexdump

#include "bat.hpp"
#include "boost/program_options.hpp"
#include "fmt/format.h"

using namespace pwa;
namespace po = boost::program_options;

namespace {

constexpr auto COLOR_NORMAL  = "\33[0m";
//constexpr auto COLOR_LIGHT   = "\33[2m";
constexpr auto COLOR_RED     = "\33[31m";
constexpr auto COLOR_BOLD    = "\33[1m";

constexpr uint32_t host2net(uint32_t hostlong)
{
    uint32_t nl = 0;
    nl |= (hostlong & 0xFF000000U) >> 24U;
    nl |= (hostlong & 0x00FF0000U) >>  8U;
    nl |= (hostlong & 0x0000FF00U) <<  8U;
    nl |= (hostlong & 0x000000FFU) << 24U;
    return nl;
}

} // end anonymous namespace

void bat::print_colorized(const std::string & col) const
{
    if (cfg.print_flags & opt::print_colors) {
        fmt::print(dst_(), "{}", col.c_str());
    }
}

void bat::print_array() const
{
    fmt::print(dst_(), "    ");
    for (std::size_t j = 0; j < cfg.bytes_on_line; ++j) {
        const auto c = quantum_[cfg.relative_offset + j];
        print_colorized(COLOR_BOLD);
        fmt::print(dst_(), "0x{:02x}{} ", c, cfg.relative_offset + j != quantum_.size() - 1 ? "," : "");
    }
}

void bat::print_hex() const
{
    cfg.colorize = true;
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const auto c = quantum_[cfg.relative_offset + j];
        cfg.colorize ^= 1;

        if (cfg.colorize)
            print_colorized(COLOR_NORMAL);
        else
            print_colorized(COLOR_BOLD);
        fmt::print(dst_(), "{:02x} ", c);
        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < (cfg.bytes_per_line)) {
        fmt::print(dst_(), "   ");
        ++j;
    }
}

void bat::print_binary() const
{
    cfg.colorize = true;
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const auto c = quantum_[cfg.relative_offset + j];
        if (j % 2)
            print_colorized(COLOR_NORMAL);
        else
            print_colorized(COLOR_BOLD);

        unsigned b = 8;
        while (b--)
            fmt::print(dst_(), "{}",
                       (static_cast<uint8_t>(c) & static_cast<uint64_t>(exp2(b)))
                       ? "1"
                       : "0");
        fmt::print(dst_(), " ");

        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < (cfg.bytes_per_line)) {
        fmt::print(dst_(), "         ");
        ++j;
    }
}

void bat::print_words() const
{
    // todo: swap word
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        if (!(j % 4))
            cfg.colorize ^= 1; // TOTO

        if (cfg.colorize)
            print_colorized(COLOR_NORMAL);
        else
            print_colorized(COLOR_BOLD);

        auto w = *reinterpret_cast<uint32_t *>(const_cast<char *>(&quantum_[cfg.relative_offset + j]));
        if (cfg.print_flags & opt::swap_endian)
            w = host2net(w);
        fmt::print(dst_(), "0x{:08x} ", w);

        j += 4;
    }

    print_colorized(COLOR_NORMAL);
    while (j < cfg.bytes_per_line) {
        fmt::print(dst_(), "           ");
        j += 4;
    }
}

void bat::print_ascii() const
{
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const auto c = quantum_[cfg.relative_offset + j];
        if (isprint(c)) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "{}", c);
        } else {
            print_colorized(COLOR_RED);
            fmt::print(dst_(), " ");
        }

        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < cfg.bytes_per_line) {
        fmt::print(dst_(), " ");
        ++j;
    }
}

// always starts on row 0:
void bat::formated_output()
{
    cfg.relative_offset = 0;
    while (cfg.relative_offset < quantum_.size()) {
        if ((cfg.relative_offset + cfg.bytes_per_line) < quantum_.size())
            cfg.bytes_on_line = cfg.bytes_per_line;
        else
            cfg.bytes_on_line = quantum_.size() - cfg.relative_offset;

        if (cfg.print_flags & opt::print_array) {
            print_array();
            fmt::print(dst_(), "\n");
            cfg.relative_offset += cfg.bytes_on_line;
            continue;
        }

        if (cfg.print_flags & opt::print_offset) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "0x{:08x} ", static_cast<uint32_t>(cfg.source_offset + cfg.relative_offset));
        }

        if (cfg.print_flags & opt::print_bin) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "|");
            print_binary();
        }

        if (cfg.print_flags & opt::print_hex) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "|");
            print_hex();
        }

        if (cfg.print_flags & opt::print_words) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "|");
            print_words();
        }

        if (cfg.print_flags & opt::print_ascii) {
            print_colorized(COLOR_NORMAL);
            fmt::print(dst_(), "|");
            print_ascii();
        }

        print_colorized(COLOR_NORMAL);
        if (cfg.print_flags & opt::print_ascii)
            fmt::print(dst_(), "|\n");
        else
            fmt::print(dst_(), "\n");

        cfg.relative_offset += cfg.bytes_on_line;
    }
}

boost::optional<config> parse_args(int argc, char ** argv)
{
    config cfg;

    // if invoked as 'cat', default to non-hex mode.
    const auto pname = [](std::string const & path) {
        return path.substr(path.find_last_of("/\\") + 1);
    }(argv[0]);
    if (pname == "cat")
        cfg.hex_mode = false;

    try {
        po::variables_map vm;
        po::options_description desc("Options");
        desc.add_options()
            ("help",            "this messages")
            ("input-file,i",    po::value<std::string>(), "input file (alt. first non-argument)")
            ("output-file,o",   po::value<std::string>(), "output file (alt. second non-argument)")
            ("count,n",         po::value<std::string>(), "read N bytes")
            ("soff,s",          po::value<std::string>(), "source file offset")
            ("doff,d",          po::value<std::string>(), "destination file offset")
            ("rows,r",          po::value<std::string>(), "number of rows in line")
            ("colors,c",        "print with colors")
            ("verbose,v",       "verbose")
            ("plain,p",         "'cat' mode, do not format output")
            ("bin,b",           "print data as binary")
            ("array,a",         "print data as C array")
            ("words,w",         "print data in machine words")
            ("endian,e",        "swap endianness")
            ("nooffset,O",      "do not print offsets")
            ("nohex,H",         "do not print hex")
            ("nobin,B",         "do not print binary")
            ("noascii,A",       "do not print ascii");

        po::positional_options_description p;
        p.add("input-file", 1);
        p.add("output-file", 1);

        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            fmt::print(stderr, "by petter wahlman, petter@wahlman.no\n");
            std::cout << desc << "\n";
            exit(EXIT_SUCCESS);
        }

        if (vm.count("input-file"))
            cfg.source_file = vm["input-file"].as<std::string>();
        if (vm.count("output-file"))
            cfg.dest_file = vm["output-file"].as<std::string>();

        if (vm.count("count")) {
            std::string tmp = vm["count"].as<std::string>();
            cfg.bytes_to_read = strtoul(tmp.c_str(), nullptr, 0);
        }
        if (vm.count("soff")) {
            std::string tmp = vm["soff"].as<std::string>();
            cfg.source_offset = strtoul(tmp.c_str(), nullptr, 0);
        }
        if (vm.count("doff")) {
            std::string tmp = vm["doff"].as<std::string>();
            cfg.dest_offset = strtoul(tmp.c_str(), nullptr, 0);
        }
        if (vm.count("rows")) {
            std::string tmp = vm["rows"].as<std::string>();
            cfg.bytes_per_line = strtoul(tmp.c_str(), nullptr, 0);
        }

        if (vm.count("plain")) {
            cfg.hex_mode = false;
        } else if (vm.count("bin")) {
            cfg.bin_mode = true;
            cfg.print_flags = opt::print_offset | opt::print_bin | opt::print_ascii;
            cfg.bytes_per_line = cfg.bytes_per_line ? cfg.bytes_per_line : 8;
        } else if (vm.count("array")) {
            cfg.hex_mode = true;
            cfg.array_mode = true;
            cfg.print_flags = opt::print_array | opt::print_hex;
        }

        if (vm.count("words"))
            cfg.print_flags |= opt::print_words;

        if (vm.count("endian"))
            cfg.print_flags |= opt::swap_endian;

        if (vm.count("verbose"))
            cfg.verbose = true;

        // turn off flags (last)
        if (vm.count("nooffset"))
            cfg.print_flags &= ~opt::print_offset;
        if (vm.count("nohex"))
            cfg.print_flags &= ~opt::print_hex;
        if (vm.count("noascii"))
            cfg.print_flags &= ~opt::print_ascii;
        if (vm.count("colors"))
            cfg.print_flags |= opt::print_colors;

    } catch (std::exception & e) {
        std::cout << e.what() << std::endl;
        return boost::optional<config>();
    }

    if (cfg.verbose) {
        fmt::print("src: offset: 0x{:08x} ({:010d}): {}\n",
               static_cast<intmax_t>(cfg.source_offset),
               static_cast<intmax_t>(cfg.source_offset),
               cfg.source_file.empty() ? "stdin" : cfg.source_file.c_str());
        fmt::print("dst: offset: 0x{:08x} ({:010d}): {}\n",
               static_cast<intmax_t>(cfg.dest_offset),
               static_cast<intmax_t>(cfg.dest_offset),
               cfg.dest_file.empty() ? "stdout" : cfg.dest_file.c_str());
    }

    return cfg;
}

bool bat::parse(std::string source_file,
                std::string dest_file,
                const std::size_t source_offset,
                const std::size_t dest_offset)
{
    src_.open(std::move(source_file), "rb", source_offset);
    source_file_size = src_.size();
    if (!src_())
        return false;

    dst_.open(std::move(dest_file), "wb", dest_offset);
    dest_file_size = dst_.size();
    if (!dst_())
        return false;

    if (cfg.array_mode)
        fmt::print(dst_(), "unsigned char data[] = {{\n");

    // chunk must be mod rows (due to offset printing, ...)
    quantum_.resize(quantum_.size() - (quantum_.size() % cfg.bytes_per_line));

    std::size_t current_offset = 0;
    do {
        const auto remaining = cfg.bytes_to_read - current_offset;
        if (!remaining)
            break;

        if (quantum_.size() > remaining)
            quantum_.resize(remaining);

        const auto bytes_read = fread(quantum_.data(), 1, quantum_.size(), src_());
        if (bytes_read < 1)
            break;

        quantum_.resize(bytes_read);
        current_offset += bytes_read;

        if (cfg.hex_mode || cfg.bin_mode) {
            formated_output();
        } else {
            if (fwrite(quantum_.data(), bytes_read, 1, dst_()) < 1)
                break;
        }
    } while (true);

    if (cfg.array_mode)
        fmt::print(dst_(), "}};\n");

    return true;
}

int main(int argc, char ** argv)
{
    auto cfg = parse_args(argc, argv);
    if (!cfg)
        return EXIT_FAILURE;

    bat bat(*cfg);
    return bat.parse(cfg->source_file,
                     cfg->dest_file,
                     cfg->source_offset,
                     cfg->dest_offset);
}
