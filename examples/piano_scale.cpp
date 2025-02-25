#include "ym2151/ym2151.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>  // std::clamp用
#include <string>

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

// 音階の周波数を計算する関数（A4 = 440Hz基準）
float noteToFrequency(int note) {
    // A4 = 69, C4 = 60
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

// YM2151のレジスタ値に変換する関数（使用しない）
uint16_t frequencyToRegisterValue(float frequency) {
    // YM2151の周波数計算は複雑ですが、簡易的に実装
    // 実際のYM2151では、キーコードとキーフラクションに分けて設定します
    return static_cast<uint16_t>(frequency * 2.0f);  // 簡易的な変換
}

// ピアノっぽい音色を設定する関数
void setupPianoVoice(YM2151::Chip& chip, int channel) {
    // アルゴリズム4（OP1->OP2->出力, OP3->OP4->出力）とフィードバック0を設定
    chip.setRegister(0x20 + channel, 4);  // アルゴリズム4, フィードバック0
    
// オペレータ1の設定（モジュレータ）
chip.setRegister(0x40 + channel, 0x7F);  // TL (Total Level) = 127 (最小音量)
chip.setRegister(0x80 + channel, 0x1F);  // AR (Attack Rate) = 31 (最速)
chip.setRegister(0xA0 + channel, 0x00);  // DR (Decay Rate) = 0 (最遅)
chip.setRegister(0xC0 + channel, 0x00);  // SR (Sustain Rate) = 0 (最遅)
chip.setRegister(0xE0 + channel, 0x0F);  // RR (Release Rate) = 15 (中速)

// オペレータ2の設定（キャリア）
chip.setRegister(0x41 + channel, 0x00);  // TL (Total Level) = 0 (最大音量)
chip.setRegister(0x81 + channel, 0x1F);  // AR (Attack Rate) = 31 (最速)
chip.setRegister(0xA1 + channel, 0x05);  // DR (Decay Rate) = 5
chip.setRegister(0xC1 + channel, 0x05);  // SR (Sustain Rate) = 5
chip.setRegister(0xE1 + channel, 0x0F);  // RR (Release Rate) = 15 (中速)

// オペレータ3の設定（モジュレータ）
chip.setRegister(0x42 + channel, 0x7F);  // TL (Total Level) = 127 (最小音量)
chip.setRegister(0x82 + channel, 0x1F);  // AR (Attack Rate) = 31 (最速)
chip.setRegister(0xA2 + channel, 0x00);  // DR (Decay Rate) = 0 (最遅)
chip.setRegister(0xC2 + channel, 0x00);  // SR (Sustain Rate) = 0 (最遅)
chip.setRegister(0xE2 + channel, 0x0F);  // RR (Release Rate) = 15 (中速)

// オペレータ4の設定（キャリア）
chip.setRegister(0x43 + channel, 0x00);  // TL (Total Level) = 0 (最大音量)
chip.setRegister(0x83 + channel, 0x1F);  // AR (Attack Rate) = 31 (最速)
chip.setRegister(0xA3 + channel, 0x05);  // DR (Decay Rate) = 5
chip.setRegister(0xC3 + channel, 0x05);  // SR (Sustain Rate) = 5
chip.setRegister(0xE3 + channel, 0x0F);  // RR (Release Rate) = 15 (中速)
    
// 倍率設定
chip.setRegister(0x60 + channel, 0x01);  // 倍率 = 1
chip.setRegister(0x61 + channel, 0x01);  // 倍率 = 1
chip.setRegister(0x62 + channel, 0x01);  // 倍率 = 1
chip.setRegister(0x63 + channel, 0x01);  // 倍率 = 1
}

// 音を鳴らす関数
void playNote(YM2151::Chip& chip, int channel, int note, float duration, std::vector<float>& buffer, int sample_rate, int& current_sample) {
    // 周波数の計算
    float frequency = noteToFrequency(note);
    
    // YM2151の周波数設定
    uint16_t freq_value = static_cast<uint16_t>(frequency * 2.0f);  // 簡易的な変換
    chip.setRegister(0x10 + channel, freq_value & 0xFF);  // 周波数下位8ビット
    chip.setRegister(0x18 + channel, freq_value >> 8);    // 周波数上位8ビット
    
    // キーオン
    chip.setRegister(0x08, 0x80 | channel);  // チャンネルのキーオン
    
    // 音声生成（キーオン期間）
    int keyOn_samples = static_cast<int>(duration * 0.8f * sample_rate);  // 80%の時間をキーオン
    for (int i = 0; i < keyOn_samples; i += 1024) {
        int samples_to_generate = std::min(1024, keyOn_samples - i);
        chip.generate(&buffer[current_sample + i], samples_to_generate);
        
        // バッファの内容を確認（デバッグ用）
        float sum = 0.0f;
        for (int j = 0; j < samples_to_generate; j++) {
            sum += std::abs(buffer[current_sample + i + j]);
        }
        std::cout << "キーオン期間のサンプル平均値: " << sum / samples_to_generate << std::endl;
    }
    
    // キーオフ
    chip.setRegister(0x08, channel);  // キーオフ
    
    // 音声生成（キーオフ期間）
    int keyOff_samples = static_cast<int>(duration * 0.2f * sample_rate);  // 20%の時間をキーオフ（リリース）
    for (int i = 0; i < keyOff_samples; i += 1024) {
        int samples_to_generate = std::min(1024, keyOff_samples - i);
        chip.generate(&buffer[current_sample + keyOn_samples + i], samples_to_generate);
        
        // バッファの内容を確認（デバッグ用）
        float sum = 0.0f;
        for (int j = 0; j < samples_to_generate; j++) {
            sum += std::abs(buffer[current_sample + keyOn_samples + i + j]);
        }
        std::cout << "キーオフ期間のサンプル平均値: " << sum / samples_to_generate << std::endl;
    }
    
    // サンプル位置の更新
    current_sample += static_cast<int>(duration * sample_rate);
}

int main() {
    // YM2151チップの初期化
    YM2151::Chip chip;
    chip.reset();
    
    // サンプリングレートの設定
    const int sample_rate = 44100;
    chip.setSampleRate(sample_rate);
    
    // ピアノっぽい音色の設定
    const int channel = 0;
    setupPianoVoice(chip, channel);
    
    // 音階の定義（C4からB4まで）
    const int notes[] = {60, 62, 64, 65, 67, 69, 71};  // C4, D4, E4, F4, G4, A4, B4
    const char* note_names[] = {"C", "D", "E", "F", "G", "A", "B"};
    const int note_count = sizeof(notes) / sizeof(notes[0]);
    
    // 各音の長さ（秒）
    const float note_duration = 0.5f;
    
    // 出力バッファの準備
    const int total_samples = static_cast<int>(note_count * note_duration * sample_rate);
    std::vector<float> output_buffer(total_samples, 0.0f);
    
    // 各音階を順番に再生
    int current_sample = 0;
    for (int i = 0; i < note_count; i++) {
        std::cout << "音階 " << note_names[i] << " を再生中..." << std::endl;
        playNote(chip, channel, notes[i], note_duration, output_buffer, sample_rate, current_sample);
    }
    
    // WAVファイルに保存
    writeWAV("ym2151_piano_scale.wav", output_buffer, 1, sample_rate, 16);
    
    return 0;
}
