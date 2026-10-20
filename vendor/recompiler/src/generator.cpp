/**
 * @file generator.cpp
 * @brief Orchestrates the code generation process
 */

#include "recompiler/rom.h"
#include "recompiler/decoder.h"
#include "recompiler/analyzer.h"
#include "recompiler/ir/ir.h"
#include "recompiler/ir/ir_builder.h"
#include "recompiler/codegen/c_emitter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>

namespace gbrecomp {

/**
 * @brief Code generator orchestrator
 * 
 * Coordinates the full recompilation pipeline:
 * 1. ROM loading and parsing
 * 2. Instruction decoding
 * 3. Control flow analysis
 * 4. IR generation
 * 5. IR optimization (optional)
 * 6. C code emission
 */
class Generator {
public:
    struct Options {
        bool verbose;
        bool emit_comments;
        bool optimize;
        bool generate_dispatch;
        std::string output_prefix;
        
        Options() : verbose(false), emit_comments(true), optimize(true), 
                    generate_dispatch(true), output_prefix("game") {}
    };
    
    explicit Generator(const Options& opts) : opts_(opts) {}
    Generator() : opts_() {}
    
    /**
     * @brief Generate code from a ROM file
     */
    bool generate(const std::filesystem::path& rom_path,
                  const std::filesystem::path& output_dir) {
        // Load ROM
        if (opts_.verbose) {
            std::cout << "Loading ROM: " << rom_path << "\n";
        }
        
        auto rom_opt = ROM::load(rom_path);
        if (!rom_opt || !rom_opt->is_valid()) {
            std::cerr << "Error: Failed to load ROM: " 
                      << (rom_opt ? rom_opt->error() : "unknown error") << "\n";
            return false;
        }
        
        const ROM& rom = *rom_opt;
        
        if (opts_.verbose) {
            print_rom_info(rom);
        }
        
        // Create decoder
        Decoder decoder(rom);
        
        // Analyze control flow
        if (opts_.verbose) {
            std::cout << "\nAnalyzing control flow...\n";
        }
        
        // Detect standard HRAM DMA routine
        // Routine: LDH (46),A; LD A,28; DEC A; JR NZ,-3; RET
        // Hex: E0 46 3E 28 3D 20 FD C9
        AnalyzerOptions analyzer_opts;
        // analyzer_opts.verbose = opts_.verbose; // analyzer options doesn't have verbose
        analyzer_opts.trace_log = false; // can be enabled if needed
        
        const std::vector<uint8_t> pattern = {0xE0, 0x46, 0x3E, 0x28, 0x3D, 0x20, 0xFD, 0xC9};
        const std::vector<uint8_t>& data = rom.bytes();
        
        auto it = std::search(data.begin(), data.end(), pattern.begin(), pattern.end());
        if (it != data.end()) {
             size_t rom_idx = std::distance(data.begin(), it);
             uint8_t bank = rom_idx / 0x4000;
             uint16_t offset = rom_idx % 0x4000;
             if (bank > 0) offset += 0x4000;
            
             if (opts_.verbose) {
                 std::cout << "Detected OAM DMA routine at ROM 0x" << std::hex << rom_idx 
                           << " (Bank " << (int)bank << ":0x" << offset << "). Mapping to HRAM 0xFF80.\n" << std::dec;
             }
             
             AnalyzerOptions::RamOverlay overlay;
             overlay.ram_addr = 0xFF80;
             overlay.rom_addr = AnalysisResult::make_addr(bank, offset);
             overlay.size = 8;
             analyzer_opts.ram_overlays.push_back(overlay);
        }

        // Use free function analyze() instead of Analyzer class
        AnalysisResult analysis = analyze(rom, analyzer_opts);
        
        if (opts_.verbose) {
            print_analysis_summary(analysis);
        }
        
        // Build IR
        if (opts_.verbose) {
            std::cout << "\nBuilding IR...\n";
        }
        
        ir::IRBuilder builder;
        ir::Program program = builder.build(analysis, rom.name());
        
        if (opts_.verbose) {
            std::cout << "Generated IR for " << program.functions.size() << " functions\n";
        }
        
        // Generate C code
        if (opts_.verbose) {
            std::cout << "\nGenerating C code...\n";
        }
        
        codegen::GeneratorOptions emit_config;
        emit_config.emit_address_comments = opts_.emit_comments;
        emit_config.emit_comments = true;
        emit_config.output_dir = output_dir.string();
        emit_config.generate_bank_dispatch = opts_.generate_dispatch;
        
        codegen::GeneratedOutput output = codegen::generate_output(
            program, rom.data(), rom.size(), emit_config);
        
        // Write output files
        if (!std::filesystem::exists(output_dir)) {
            std::filesystem::create_directories(output_dir);
        }
        
        if (!codegen::write_output(output, output_dir.string())) {
            std::cerr << "Error: Failed to write output files\n";
            return false;
        }
        
        if (opts_.verbose) {
            std::cout << "Output written to: " << output_dir << "\n";
            std::cout << "  - " << output.header_file << "\n";
            std::cout << "  - " << output.source_file << "\n";
            std::cout << "  - " << output.rom_data_file << "\n";
        }
        
        return true;
    }
    
private:
    Options opts_;
};

} // namespace gbrecomp
