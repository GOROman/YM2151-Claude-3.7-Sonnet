#include "ym2151/ym2151.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>  // std::clamp用

// WAVファイルヘッダー構造体
struct WAVHeader {
    // RIFFチャンク
    char riff_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_size;
    char wave_id[4] = {'W', 'A', 'V', 'E'};
    
    // fmtチャンク
    char fmt_id[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t format = 1;  // PCM
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    
    // dataチャンク
    char data_id[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};

// WAVファイルに書き込む関数
void writeWAV(const std::string& filename, const std::vector<float>& samples, int channels, int sample_rate, int bits_per_sample) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "ファイルを開けませんでした: " << filename << std::endl;
        return;
    }
    
    WAVHeader header;
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = bits_per_sample;
    header.block_align = channels * bits_per_sample / 8;
    header.byte_rate = sample_rate * header.block_align;
    header.data_size = samples.size() * header.block_align;
    header.riff_size = 36 + header.data_size;
    
    // ヘッダーの書き込み
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // サンプルデータの書き込み
    if (bits_per_sample == 16) {
        for (float sample : samples) {
            // 浮動小数点を16ビット整数に変換
            int16_t pcm = static_cast<int16_t>(std::clamp(sample * 32767.0f, -32768.0f, 32767.0f));
            file.write(reinterpret_cast<const char*>(&pcm), sizeof(pcm));
        }
    } else if (bits_per_sample == 32) {
        for (float sample : samples) {
            // 32ビット浮動小数点として書き込み
            file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        }
    }
    
    file.close();
    std::cout << "WAVファイルを保存しました: " << filename << std::endl;
}

int main() {
    // YM2151チップの初期化
    YM2151::Chip chip;
    chip.reset();
    
    // サンプリングレートの設定
    const int sample_rate = 44100;
    chip.setSampleRate(sample_rate);
    
    // 音色の設定（シンプルなFM音色）
    // チャンネル0を使用
    const int channel = 0;
    
    // アルゴリズム7（OP4のみ出力）とフィードバック0を設定
    chip.setRegister(0x20 + channel, 7);  // アルゴリズム7, フィードバック0
    
    // オペレータ1の設定（使用しない）
    chip.setRegister(0x40, 0x7F);  // TL (Total Level) = 127 (最小音量)
    chip.setRegister(0x80, 0x1F);  // AR (Attack Rate) = 31 (最速)
    chip.setRegister(0xA0, 0x00);  // DR (Decay Rate) = 0
    chip.setRegister(0xC0, 0x00);  // SR (Sustain Rate) = 0
    chip.setRegister(0xE0, 0x0F);  // RR (Release Rate) = 15
    
    // オペレータ2の設定（使用しない）
    chip.setRegister(0x41, 0x7F);  // TL (Total Level) = 127 (最小音量)
    chip.setRegister(0x81, 0x1F);  // AR (Attack Rate) = 31 (最速)
    chip.setRegister(0xA1, 0x00);  // DR (Decay Rate) = 0
    chip.setRegister(0xC1, 0x00);  // SR (Sustain Rate) = 0
    chip.setRegister(0xE1, 0x0F);  // RR (Release Rate) = 15
    
    // オペレータ3の設定（使用しない）
    chip.setRegister(0x42, 0x7F);  // TL (Total Level) = 127 (最小音量)
    chip.setRegister(0x82, 0x1F);  // AR (Attack Rate) = 31 (最速)
    chip.setRegister(0xA2, 0x00);  // DR (Decay Rate) = 0
    chip.setRegister(0xC2, 0x00);  // SR (Sustain Rate) = 0
    chip.setRegister(0xE2, 0x0F);  // RR (Release Rate) = 15
    
    // オペレータ4の設定（キャリア）
    chip.setRegister(0x43, 0x00);  // TL (Total Level) = 0 (最大音量)
    chip.setRegister(0x83, 0x1F);  // AR (Attack Rate) = 31 (最速)
    chip.setRegister(0xA3, 0x00);  // DR (Decay Rate) = 0
    chip.setRegister(0xC3, 0x00);  // SR (Sustain Rate) = 0
    chip.setRegister(0xE3, 0x0F);  // RR (Release Rate) = 15
    
    // マルチプライヤーの設定
    chip.setRegister(0x30, 0x01);  // OP1: MULT=1
    chip.setRegister(0x31, 0x01);  // OP2: MULT=1
    chip.setRegister(0x32, 0x01);  // OP3: MULT=1
    chip.setRegister(0x33, 0x01);  // OP4: MULT=1
    
    // A4（440Hz）の設定
    float frequency = 440.0f;  // A4 = 440Hz
    uint16_t freq_value = static_cast<uint16_t>(frequency * 2.0f);  // 簡易的な変換
    
    // 周波数レジスタの設定
    chip.setRegister(0x10 + channel, freq_value & 0xFF);  // 周波数下位8ビット
    chip.setRegister(0x18 + channel, freq_value >> 8);    // 周波数上位8ビット
    
    // キーオン
    chip.setRegister(0x08, 0x80 | channel);  // チャンネル0のキーオン
    
    std::cout << "Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "Frequency Register Value: 0x" << std::hex << freq_value << std::dec << std::endl;
    
    // 音声生成
    const int duration_seconds = 3;
    const int total_samples = sample_rate * duration_seconds;
    std::vector<float> output_buffer(total_samples);
    
    // バッファに音声データを生成
    for (int i = 0; i < total_samples; i += 1024) {
        int samples_to_generate = std::min(1024, total_samples - i);
        chip.generate(&output_buffer[i], samples_to_generate);
        
        // バッファの内容を確認（デバッグ用）
        float sum = 0.0f;
        for (int j = 0; j < samples_to_generate; j++) {
            sum += std::abs(output_buffer[i + j]);
        }
        std::cout << "Sample block " << i/1024 << " average value: " << sum / samples_to_generate << std::endl;
    }
    
    // 1秒後にキーオフ
    const int keyoff_sample = sample_rate;
    if (keyoff_sample < total_samples) {
        chip.setRegister(0x08, channel);  // チャンネル0のキーオフ
        
        // キーオフ後も音声生成を続ける
        for (int i = keyoff_sample; i < total_samples; i += 1024) {
            int samples_to_generate = std::min(1024, total_samples - i);
            chip.generate(&output_buffer[i], samples_to_generate);
        }
    }
    
    // WAVファイルに保存
    writeWAV("ym2151_tone.wav", output_buffer, 1, sample_rate, 16);
    
    return 0;
}
