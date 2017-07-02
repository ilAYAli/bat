// bat -- combined cat, dd and hexdump

#include <errno.h>
#include "bat.hpp"

#ifdef _WIN32
    #define fileno _fileno
    bool isatty(int) { return true; }
    void umask(int) { }
#endif

#include "boost/program_options.hpp"
#include <iostream>
#include <sys/stat.h>

namespace po = boost::program_options;

namespace {

// htonl, windows compatible:
uint32_t host2net(uint32_t hostlong)
{
    uint32_t nl = 0;
    nl |= (hostlong & 0xFF000000) >> 24;
    nl |= (hostlong & 0x00FF0000) >> 8;
    nl |= (hostlong & 0x0000FF00) << 8;
    nl |= (hostlong & 0x000000FF) << 24;
    return nl;
}

FILE * open_source_file(const std::string & filename, std::size_t offset, std::size_t & size)
{
    try {
        if (filename.empty())
            return stdin;

        auto fp = fopen(filename.c_str(), "rb");
        if (!fp) {
            perror(filename.c_str());
            throw "could not open file";
        }

        struct stat st;
        fstat(fileno(fp), &st);
        size = st.st_size;

        if (isatty(fileno(stdin)) && (fseek(fp, offset, SEEK_SET) == -1))
            throw "failed to seek";

        return fp;
    } catch (std::string & what) {
        std::cout << "error, " << what << std::endl;
    }
    return nullptr;
}

FILE * open_dest_file(const std::string & filename, std::size_t offset, std::size_t & size)
{
    try {
        if (filename.empty() || !strcmp(filename.c_str(), "-"))
            return stdout;

        umask(022);
        auto fp = fopen(filename.c_str(), "wb");
        if (!fp)
            throw "could not open file";

        struct stat st;
        fstat(fileno(fp), &st);
        if (offset > st.st_size)
            ;//throw "offset too large";
        size = st.st_size;

        if (offset != fseek(fp, offset, SEEK_SET))
            ;//throw "unable to seek";

        return fp;
    } catch (const char * what) {
        std::cout << "error, " << what << std::endl;
    }
    return nullptr;
}

} // anon ns

bat::~bat()
{
    if (fp_ != stdin)
        fclose(fp_);

    fflush(fpo_);
    if (fpo_ != stdout)
        fclose(fpo_);
}

void bat::print_colorized(const std::string & col) const
{
    if (cfg.print_flags & PRINT_COLORS)
        fprintf(fpo_, "%s", col.c_str());
}

void bat::print_array() const
{
    fprintf(fpo_, "    ");
    for (std::size_t j = 0; j < cfg.bytes_on_line; ++j) {
        const unsigned char c = quantum_[cfg.relative_offset + j];
        print_colorized(COLOR_LIGHT);
        fprintf(fpo_, "0x%02x%s ", c, cfg.relative_offset + j != quantum_.size() - 1 ? "," : "");
    }
}

void bat::print_hex() const
{
    cfg.colorize = true;
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const unsigned char c = quantum_[cfg.relative_offset + j];
        cfg.colorize ^= 1;

        if (cfg.colorize)
            print_colorized(COLOR_NORMAL);
        else
            print_colorized(COLOR_LIGHT);

        fprintf(fpo_, "%02x", c);

        fputc(' ', fpo_);

        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < (cfg.bytes_per_line)) {
        fprintf(fpo_, "   ");
        ++j;
    }
}

void bat::print_binary() const
{
    cfg.colorize = true;
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const unsigned char c = quantum_[cfg.relative_offset + j];

        if (j % 2)
            print_colorized(COLOR_NORMAL);
        else
            print_colorized(COLOR_LIGHT);

        unsigned b = 8;
        while (b--)
            fputc((c & (unsigned long long)exp2(b)) ? '1' : '0', fpo_);
        fputc(' ', fpo_);

        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < (cfg.bytes_per_line)) {
        fprintf(fpo_, "         ");
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
            print_colorized(COLOR_LIGHT);

        unsigned int w = *(unsigned int *)(char *)&quantum_[cfg.relative_offset + j];
        if (cfg.print_flags & SWAP_ENDIAN)
            w = host2net(w);
        fprintf(fpo_, "0x%08x ", w);

        j += 4;
    }

    print_colorized(COLOR_NORMAL);
    while (j < cfg.bytes_per_line) {
        fprintf(fpo_, "           ");
        j += 4;
    }
}

void bat::print_ascii() const
{
    std::size_t j = 0;
    while (j < cfg.bytes_on_line) {
        const unsigned char c = quantum_[cfg.relative_offset + j];

        if (isprint(c)) {
            print_colorized(COLOR_NORMAL);
            fputc(c, fpo_);
        } else {
            print_colorized(COLOR_RED);
            fputc('.', fpo_);
        }

        ++j;
    }

    print_colorized(COLOR_NORMAL);
    while (j < cfg.bytes_per_line) {
        fprintf(fpo_, " ");
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

        if (cfg.print_flags & PRINT_ARRAY) {
            print_array();
            fprintf(fpo_, "\n");
            cfg.relative_offset += cfg.bytes_on_line;
            continue;
        }

        if (cfg.print_flags & PRINT_OFFSET) {
            print_colorized(COLOR_NORMAL);
            fprintf(fpo_, "0x%08x ", static_cast<unsigned>(cfg.source_offset + cfg.relative_offset));
        }

        if (cfg.print_flags & PRINT_BIN) {
            print_colorized(COLOR_NORMAL);
            fputc('|', fpo_);
            print_binary();
        }

        if (cfg.print_flags & PRINT_HEX) {
            print_colorized(COLOR_NORMAL);
            fputc('|', fpo_);
            print_hex();
        }

        if (cfg.print_flags & PRINT_WORDS) {
            print_colorized(COLOR_NORMAL);
            fputc('|', fpo_);
            print_words();
        }

        if (cfg.print_flags & PRINT_ASCII) {
            print_colorized(COLOR_NORMAL);
            fputc('|', fpo_);
            print_ascii();
        }

        print_colorized(COLOR_NORMAL);
        if (cfg.print_flags & PRINT_ASCII)
            fprintf(fpo_, "|\n");
        else
            fputc('\n', fpo_);

        cfg.relative_offset += cfg.bytes_on_line;
    }
}

boost::optional<config> parse_args(int argc, char ** argv)
{
    config cfg;
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
            fprintf(stderr, "by petter wahlman, petter@wahlman.no\n");
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
            cfg.print_flags = PRINT_OFFSET | PRINT_BIN | PRINT_ASCII;
            cfg.bytes_per_line = cfg.bytes_per_line ? cfg.bytes_per_line : 8;
        } else if (vm.count("array")) {
            cfg.hex_mode = true;
            cfg.array_mode = true;
            cfg.print_flags = PRINT_ARRAY | PRINT_HEX;
        }

        if (vm.count("words"))
            cfg.print_flags |= PRINT_WORDS;

        if (vm.count("endian"))
            cfg.print_flags |= SWAP_ENDIAN;

        if (vm.count("verbose"))
            cfg.verbose = true;

        // turn off flags (last)
        if (vm.count("nooffset"))
            cfg.print_flags &= ~PRINT_OFFSET;
        if (vm.count("nohex"))
            cfg.print_flags &= ~PRINT_HEX;
        if (vm.count("noascii"))
            cfg.print_flags &= ~PRINT_ASCII;
        if (vm.count("colors"))
            cfg.print_flags |= PRINT_COLORS;

    } catch (po::error & e) {
        std::cout << e.what() << std::endl;
        return boost::optional<config>();
    }

    if (cfg.verbose) {
        printf("src: offset: 0x%08jx (%.10jd): %s\n",
               (intmax_t)cfg.source_offset,
               (intmax_t)cfg.source_offset,
               cfg.source_file.empty() ? "stdin" : cfg.source_file.c_str());
        printf("dst: offset: 0x%08jx (%.10jd): %s\n\n",
               (intmax_t)cfg.dest_offset,
               (intmax_t)cfg.dest_offset,
               cfg.dest_file.empty() ? "stdout" : cfg.dest_file.c_str());
    }

    return cfg;
}

bool bat::parse(const std::string & source_file,
                const std::string & dest_file,
                const std::size_t source_offset,
                const std::size_t dest_offset)
{
    fp_ = open_source_file(source_file, source_offset, source_file_size);
    if (!fp_) return false;

    fpo_ = open_dest_file(dest_file, dest_offset, dest_file_size);
    if (!fpo_) return false;

    if (cfg.array_mode)
        fprintf(fpo_, "unsigned char data[] = {\n");

    // chunk must be mod rows (due to offset printing, ...)
    quantum_.resize(quantum_.size() - (quantum_.size() % cfg.bytes_per_line));

    std::size_t current_offset = 0;
    do {
        const auto remaining = cfg.bytes_to_read - current_offset;
        if (!remaining)
            break;

        if (quantum_.size() > remaining)
            quantum_.resize(remaining);

        const auto bytes_read = fread(quantum_.data(), 1, quantum_.size(), fp_);
        if (bytes_read < 1)
            break;

        quantum_.resize(bytes_read);
        current_offset += bytes_read;

        if (cfg.hex_mode || cfg.bin_mode) {
            formated_output();
        } else {
            if (fwrite(quantum_.data(), bytes_read, 1, fpo_) < 1)
                break;
        }
    } while (true);

    if (cfg.array_mode)
        fprintf(fpo_, "};\n");

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
