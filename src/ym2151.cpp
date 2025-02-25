#include "ym2151/ym2151.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace YM2151 {

// 定数定義
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

// サイン波テーブル
constexpr int SINE_TABLE_SIZE = 1024;
std::array<float, SINE_TABLE_SIZE> sine_table;

// エンベロープ生成用の定数
constexpr float ATTACK_RATE_FACTOR = 0.001f;
constexpr float DECAY_RATE_FACTOR = 0.0001f;
constexpr float SUSTAIN_RATE_FACTOR = 0.00005f;
constexpr float RELEASE_RATE_FACTOR = 0.0002f;

// アルゴリズム接続テーブル（YM2151は8種類のアルゴリズムを持つ）
constexpr std::array<std::array<int, 4>, 8> algorithm_connection = {{
    {0, 1, 2, 3},  // アルゴリズム0: OP1->OP2->OP3->OP4->出力
    {0, 1, 3, 2},  // アルゴリズム1: OP1->OP2->OP4->出力, OP3->出力
    {0, 3, 1, 2},  // アルゴリズム2: OP1->OP3->OP4->出力, OP2->出力
    {0, 3, 2, 1},  // アルゴリズム3: OP1->OP3->出力, OP2->OP4->出力
    {0, 2, 1, 3},  // アルゴリズム4: OP1->OP2->出力, OP3->OP4->出力
    {0, 2, 3, 1},  // アルゴリズム5: OP1->OP2->出力, OP3->出力, OP4->出力
    {0, 1, 2, 3},  // アルゴリズム6: OP1->出力, OP2->OP3->出力, OP4->出力
    {0, 1, 2, 3}   // アルゴリズム7: OP1->出力, OP2->出力, OP3->出力, OP4->出力
}};

// サイン波テーブルの初期化
void initSineTable() {
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < SINE_TABLE_SIZE; ++i) {
            sine_table[i] = std::sin(TWO_PI * i / SINE_TABLE_SIZE);
        }
        initialized = true;
    }
}

// サイン波の取得（テーブル参照）
float getSine(float phase) {
    // テーブルが初期化されていない場合は初期化
    initSineTable();
    
    // 位相をインデックスに変換
    int index = static_cast<int>(phase * SINE_TABLE_SIZE / TWO_PI) & (SINE_TABLE_SIZE - 1);
    
    // デバッグ出力
    static bool debug_printed = false;
    if (!debug_printed) {
        std::cout << "getSine - phase: " << phase << ", index: " << index << ", value: " << sine_table[index] << std::endl;
        debug_printed = true;
    }
    
    return sine_table[index];
}

// Operator実装
Operator::Operator() : envelope_(0.0f), phase_(0.0f), output_(0.0f), 
                       env_state_(EnvelopeState::IDLE), env_level_(0.0f), env_rate_(0.0f) {
    initSineTable();
    reset();
}

Operator::~Operator() {
}

void Operator::reset() {
    envelope_ = 0.0f;
    phase_ = 0.0f;
    output_ = 0.0f;
    
    // エンベロープ状態のリセット
    env_state_ = EnvelopeState::IDLE;
    env_level_ = 0.0f;
    env_rate_ = 0.0f;
    
    // デフォルトパラメータの設定
    params_.dt1 = 0;
    params_.mul = 1;
    params_.tl = 127;  // 最小音量
    params_.ks = 0;
    params_.ar = 31;   // 最速アタック
    params_.amsen = 0;
    params_.dr = 0;
    params_.dt2 = 0;
    params_.sr = 0;
    params_.sl = 0;
    params_.rr = 15;   // 中速リリース
    params_.ssgeg = false;
}

void Operator::setParameter(const FMParameter& param) {
    params_ = param;
}

void Operator::keyOn() {
    // アタックフェーズの開始
    env_state_ = EnvelopeState::ATTACK;
    
    // アタックレートの設定（より速い応答のために調整）
    env_rate_ = params_.ar * ATTACK_RATE_FACTOR * 10.0f;
    
    // アタックが最速の場合は即座に最大レベルに
    if (params_.ar == 31) {
        env_level_ = 1.0f;
        env_state_ = EnvelopeState::DECAY;
        env_rate_ = params_.dr * DECAY_RATE_FACTOR;
    } else {
        // アタックレートが最速でない場合も、より高い初期レベルを設定
        env_level_ = 0.8f;
    }
    
    // エンベロープ値を即座に反映
    envelope_ = env_level_;
    
    // デバッグ出力
    std::cout << "Operator::keyOn - env_level: " << env_level_ << ", env_rate: " << env_rate_ << ", envelope: " << envelope_ << std::endl;
}

void Operator::keyOff() {
    // リリースフェーズの開始
    env_state_ = EnvelopeState::RELEASE;
    
    // リリースレートの設定
    env_rate_ = params_.rr * RELEASE_RATE_FACTOR;
}

void Operator::updateEnvelope() {
    // サスティンレベルを事前に計算
    float sustain_level = 1.0f - (params_.sl / 15.0f);
    
    // エンベロープ状態に応じた処理
    switch (env_state_) {
        case EnvelopeState::IDLE:
            // アイドル状態では何もしない
            break;
            
        case EnvelopeState::ATTACK:
            // アタックフェーズ：指数関数的に上昇
            env_level_ += (1.0f - env_level_) * env_rate_;
            
            // 閾値に達したらディケイフェーズへ
            if (env_level_ > 0.99f) {
                env_level_ = 1.0f;
                env_state_ = EnvelopeState::DECAY;
                env_rate_ = params_.dr * DECAY_RATE_FACTOR;
            }
            break;
            
        case EnvelopeState::DECAY:
            // ディケイフェーズ：指数関数的に減衰
            env_level_ -= env_level_ * env_rate_;
            
            // サスティンレベルに達したらサスティンフェーズへ
            if (env_level_ <= sustain_level) {
                env_level_ = sustain_level;
                env_state_ = EnvelopeState::SUSTAIN;
                env_rate_ = params_.sr * SUSTAIN_RATE_FACTOR;
            }
            break;
            
        case EnvelopeState::SUSTAIN:
            // サスティンフェーズ：緩やかに減衰
            env_level_ -= env_level_ * env_rate_;
            
            // 最小レベルに達したら停止
            if (env_level_ < 0.001f) {
                env_level_ = 0.0f;
                env_state_ = EnvelopeState::IDLE;
            }
            break;
            
        case EnvelopeState::RELEASE:
            // リリースフェーズ：指数関数的に減衰
            env_level_ -= env_level_ * env_rate_;
            
            // 最小レベルに達したら停止
            if (env_level_ < 0.001f) {
                env_level_ = 0.0f;
                env_state_ = EnvelopeState::IDLE;
            }
            break;
    }
    
    // エンベロープレベルをエンベロープ値に適用（より強い出力のために調整）
    envelope_ = env_level_ * 2.0f;  // 出力を2倍に増幅
}

float Operator::getOutput(float phase, float modulation) {
    // デチューン値の適用
    float detune = params_.dt1 * 0.05f + params_.dt2 * 0.1f;
    
    // 周波数乗数の適用
    float frequency_multiplier = params_.mul ? params_.mul : 0.5f;
    
    // 位相計算（変調を含む）
    float current_phase = phase * frequency_multiplier + detune + modulation;
    
    // 位相を0〜2πの範囲に正規化
    while (current_phase >= TWO_PI) current_phase -= TWO_PI;
    while (current_phase < 0) current_phase += TWO_PI;
    
    // サイン波生成
    float sine_value = getSine(current_phase);
    
    // エンベロープの更新
    updateEnvelope();
    
    // エンベロープの適用（トータルレベルも考慮）
    // トータルレベルを計算（0が最大音量、127が最小音量）
    float level = 1.0f;  // デバッグ用に一時的に1.0に設定
    
    // 出力レベルを調整（音量を適切なレベルに調整）
    output_ = sine_value * envelope_ * level * 8192.0f;
    
    // デバッグ出力
    static bool debug_printed = false;
    if (!debug_printed) {
        std::cout << "Operator::getOutput - phase: " << current_phase << ", sine: " << sine_value
                  << ", env: " << envelope_ << ", level: " << level
                  << ", output: " << output_ << std::endl;
        debug_printed = true;
    }
    
    return output_;
}

// Channel実装
Channel::Channel() : frequency_(0), algorithm_(0), feedback_(0), sample_rate_(44100), keyOnFlag_(false), output_(0.0f), phase_accumulator_(0.0f) {
    feedback_buffer_[0] = 0.0f;
    feedback_buffer_[1] = 0.0f;
    reset();
}

Channel::~Channel() {
}

void Channel::reset() {
    for (auto& op : operators_) {
        op.reset();
    }
    frequency_ = 0;
    algorithm_ = 0;
    feedback_ = 0;
    sample_rate_ = 44100;  // デフォルトサンプリングレート
    keyOnFlag_ = false;
    output_ = 0.0f;
    feedback_buffer_[0] = 0.0f;
    feedback_buffer_[1] = 0.0f;
    phase_accumulator_ = 0.0f;  // 位相累積変数の初期化
}

void Channel::setSampleRate(uint32_t rate) {
    sample_rate_ = rate;
}

void Channel::setFrequency(uint16_t frequency) {
    frequency_ = frequency;
}

void Channel::setAlgorithm(uint8_t algorithm) {
    algorithm_ = algorithm & 0x07;  // 0-7の範囲に制限
}

void Channel::setFeedback(uint8_t feedback) {
    feedback_ = feedback & 0x07;  // 0-7の範囲に制限
}

void Channel::keyOn() {
    keyOnFlag_ = true;
    
    // 各オペレータのキーオン処理
    for (auto& op : operators_) {
        op.keyOn();
    }
    
    // デバッグ出力
    std::cout << "Channel::keyOn - frequency: " << frequency_ << ", algorithm: " << static_cast<int>(algorithm_) << std::endl;
}

void Channel::keyOff() {
    keyOnFlag_ = false;
    
    // 各オペレータのキーオフ処理
    for (auto& op : operators_) {
        op.keyOff();
    }
}

void Channel::updateEnvelopes() {
    // 各オペレータのエンベロープ更新
    for (auto& op : operators_) {
        op.updateEnvelope();
    }
}

Operator& Channel::getOperator(int index) {
    return operators_[index & 0x03];  // 0-3の範囲に制限
}

float Channel::getOutput() {
    // 基本位相の計算（累積）
    float phase_increment = TWO_PI * frequency_ / static_cast<float>(sample_rate_);
    phase_accumulator_ += phase_increment;
    if (phase_accumulator_ >= TWO_PI) phase_accumulator_ -= TWO_PI;
    
    // フィードバック値の計算
    float feedback = 0.0f;
    if (feedback_ > 0) {
        feedback = (feedback_buffer_[0] + feedback_buffer_[1]) * (feedback_ * 0.1f);
    }
    
    // アルゴリズムに基づいて各オペレータの出力を計算
    float op_outputs[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    // キーオンフラグがfalseの場合は0を返す
    if (!keyOnFlag_) {
        return 0.0f;
    }
    
    // デバッグ出力
    std::cout << "Channel::getOutput - phase_accumulator: " << phase_accumulator_ << ", frequency: " << frequency_ << std::endl;
    
    // アルゴリズムに応じた接続パターンで計算
    switch (algorithm_) {
        case 0:  // OP1->OP2->OP3->OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, op_outputs[1]);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, op_outputs[2]);
            output_ = op_outputs[3];
            break;
            
        case 1:  // OP1->OP2->OP4->出力, OP3->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, 0.0f);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, op_outputs[1]);
            output_ = op_outputs[2] + op_outputs[3];
            break;
            
        case 2:  // OP1->OP3->OP4->出力, OP2->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, 0.0f);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, op_outputs[2]);
            output_ = op_outputs[1] + op_outputs[3];
            break;
            
        case 3:  // OP1->OP3->出力, OP2->OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, 0.0f);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, op_outputs[1]);
            output_ = op_outputs[2] + op_outputs[3];
            break;
            
        case 4:  // OP1->OP2->出力, OP3->OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, 0.0f);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, op_outputs[2]);
            output_ = op_outputs[1] + op_outputs[3];
            
            // デバッグ出力
            std::cout << "Algorithm 4 - op_outputs: " << op_outputs[0] << ", " << op_outputs[1] << ", " << op_outputs[2] << ", " << op_outputs[3] << ", output: " << output_ << std::endl;
            break;
            
        case 5:  // OP1->OP2->出力, OP3->出力, OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, op_outputs[0]);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, 0.0f);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, 0.0f);
            output_ = op_outputs[1] + op_outputs[2] + op_outputs[3];
            break;
            
        case 6:  // OP1->出力, OP2->OP3->出力, OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, 0.0f);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, op_outputs[1]);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, 0.0f);
            output_ = op_outputs[0] + op_outputs[2] + op_outputs[3];
            break;
            
        case 7:  // OP1->出力, OP2->出力, OP3->出力, OP4->出力
            op_outputs[0] = operators_[0].getOutput(phase_accumulator_, feedback);
            op_outputs[1] = operators_[1].getOutput(phase_accumulator_, 0.0f);
            op_outputs[2] = operators_[2].getOutput(phase_accumulator_, 0.0f);
            op_outputs[3] = operators_[3].getOutput(phase_accumulator_, 0.0f);
            output_ = op_outputs[0] + op_outputs[1] + op_outputs[2] + op_outputs[3];
            
            // デバッグ出力
            std::cout << "Algorithm 7 - op_outputs: " << op_outputs[0] << ", " << op_outputs[1] << ", " << op_outputs[2] << ", " << op_outputs[3] << ", output: " << output_ << std::endl;
            break;
            
        default:
            // デフォルトケース（単純なサイン波）
            output_ = std::sin(phase_accumulator_);
            break;
    }
    
    // フィードバックバッファの更新
    feedback_buffer_[1] = feedback_buffer_[0];
    feedback_buffer_[0] = op_outputs[0];
    
    return output_;
}

// Chip実装
Chip::Chip(uint32_t clock) : 
    clock_(clock), 
    sample_rate_(44100),  // デフォルトサンプリングレート
    timer_a_val_(0),
    timer_b_val_(0),
    timer_a_enabled_(false),
    timer_b_enabled_(false),
    timer_a_overflow_(false),
    timer_b_overflow_(false),
    lfo_frequency_(0),
    lfo_waveform_(0),
    lfo_phase_(0.0f),
    lfo_am_depth_(0.0f),
    lfo_pm_depth_(0.0f) {
    
    reset();
}

Chip::~Chip() {
}

void Chip::reset() {
    // レジスタの初期化
    for (auto& reg : registers_) {
        reg = 0;
    }
    
    // チャンネルの初期化
    for (auto& channel : channels_) {
        channel.reset();
        channel.setSampleRate(sample_rate_);
    }
    
    // タイマーの初期化
    timer_a_val_ = 0;
    timer_b_val_ = 0;
    timer_a_enabled_ = false;
    timer_b_enabled_ = false;
    timer_a_overflow_ = false;
    timer_b_overflow_ = false;
    
    // LFOの初期化
    lfo_frequency_ = 0;
    lfo_waveform_ = 0;
    lfo_phase_ = 0.0f;
    lfo_am_depth_ = 0.0f;
    lfo_pm_depth_ = 0.0f;
}

void Chip::setRegister(uint8_t reg, uint8_t value) {
    registers_[reg] = value;
    
    // レジスタ値に基づいて内部状態を更新
    switch (reg) {
        case 0x01:  // LFO周波数
            lfo_frequency_ = value & 0x0F;
            break;
            
        case 0x08:  // キーオン/オフ
            {
                uint8_t channel = value & 0x07;
                uint8_t slot = (value >> 3) & 0x03;
                bool key_on = (value & 0x80) != 0;
                
                if (key_on) {
                    channels_[channel].keyOn();
                } else {
                    channels_[channel].keyOff();
                }
            }
            break;
            
        case 0x0F:  // ノイズイネーブル、ノイズ周波数
            // ノイズ機能の実装（省略）
            break;
            
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:  // チャンネル周波数（下位8ビット）
            {
                uint8_t channel = reg & 0x07;
                uint16_t freq = (registers_[0x18 + channel] << 8) | value;
                channels_[channel].setFrequency(freq);
            }
            break;
            
        case 0x18: case 0x19: case 0x1A: case 0x1B:
        case 0x1C: case 0x1D: case 0x1E: case 0x1F:  // チャンネル周波数（上位8ビット）
            {
                uint8_t channel = reg & 0x07;
                uint16_t freq = (value << 8) | registers_[0x10 + channel];
                channels_[channel].setFrequency(freq);
            }
            break;
            
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x24: case 0x25: case 0x26: case 0x27:  // アルゴリズム、フィードバック
            {
                uint8_t channel = reg & 0x07;
                uint8_t algorithm = value & 0x07;
                uint8_t feedback = (value >> 3) & 0x07;
                channels_[channel].setAlgorithm(algorithm);
                channels_[channel].setFeedback(feedback);
            }
            break;
            
        // オペレータパラメータの設定（省略）
        // 実際の実装では、各オペレータのパラメータを設定するコードが必要
            
        default:
            // その他のレジスタ処理
            break;
    }
}

uint8_t Chip::getRegister(uint8_t reg) const {
    return registers_[reg];
}

void Chip::setSampleRate(uint32_t rate) {
    sample_rate_ = rate;
    
    // 各チャンネルにもサンプリングレートを設定
    for (auto& channel : channels_) {
        channel.setSampleRate(rate);
    }
}

Channel& Chip::getChannel(int index) {
    return channels_[index & 0x07];  // 0-7の範囲に制限
}

void Chip::updateTimers() {
    // タイマーの更新処理（省略）
}

void Chip::updateLFO() {
    // LFOの更新処理
    if (lfo_frequency_ > 0) {
        float lfo_step = lfo_frequency_ * 0.01f / sample_rate_;
        lfo_phase_ += lfo_step;
        if (lfo_phase_ >= 1.0f) lfo_phase_ -= 1.0f;
    }
}

float Chip::getLFOValue() {
    // LFO波形の生成
    float lfo_value = 0.0f;
    
    switch (lfo_waveform_) {
        case 0:  // 三角波
            if (lfo_phase_ < 0.5f) {
                lfo_value = lfo_phase_ * 2.0f;
            } else {
                lfo_value = 2.0f - lfo_phase_ * 2.0f;
            }
            break;
            
        case 1:  // ノコギリ波
            lfo_value = lfo_phase_;
            break;
            
        case 2:  // 矩形波
            lfo_value = (lfo_phase_ < 0.5f) ? 1.0f : 0.0f;
            break;
            
        case 3:  // ランダム
            // 簡易的な疑似ランダム
            lfo_value = static_cast<float>(std::rand()) / RAND_MAX;
            break;
    }
    
    return lfo_value;
}

void Chip::generate(float* buffer, int samples) {
    for (int i = 0; i < samples; ++i) {
        // タイマーとLFOの更新
        updateTimers();
        updateLFO();
        
        // 全チャンネルの出力を合成
        float output = 0.0f;
        for (auto& channel : channels_) {
            // エンベロープの更新はgetOutput内で行われるため不要
            output += channel.getOutput();
        }
        
        // 出力レベルを調整（音量を大きくする）
        output *= 100.0f;
        
        // 出力バッファに書き込み
        buffer[i] = output;
    }
}

} // namespace YM2151
