#ifndef YM2151_H
#define YM2151_H

#include <cstdint>
#include <vector>
#include <array>
#include <memory>

namespace YM2151 {

// YM2151のレジスタ数
constexpr int REGISTER_COUNT = 256;

// オペレータの数
constexpr int OPERATOR_COUNT = 32;

// チャンネル数
constexpr int CHANNEL_COUNT = 8;

// FM音源のパラメータ構造体
struct FMParameter {
    uint8_t dt1;    // Detune 1
    uint8_t mul;    // Frequency Multiplier
    uint8_t tl;     // Total Level
    uint8_t ks;     // Key Scale
    uint8_t ar;     // Attack Rate
    uint8_t amsen;  // AM Sensitivity
    uint8_t dr;     // Decay Rate
    uint8_t dt2;    // Detune 2
    uint8_t sr;     // Sustain Rate
    uint8_t sl;     // Sustain Level
    uint8_t rr;     // Release Rate
    bool    ssgeg;  // SSG-EG
};

// オペレータクラス
class Operator {
public:
    Operator();
    ~Operator();

    void reset();
    void setParameter(const FMParameter& param);
    float getOutput(float phase, float modulation);

private:
    FMParameter params_;
    float envelope_;
    float phase_;
    float output_;
};

// チャンネルクラス
class Channel {
public:
    Channel();
    ~Channel();

    void reset();
    void setFrequency(uint16_t frequency);
    void setAlgorithm(uint8_t algorithm);
    void setFeedback(uint8_t feedback);
    void keyOn();
    void keyOff();
    float getOutput();

    Operator& getOperator(int index);

private:
    std::array<Operator, 4> operators_;
    uint16_t frequency_;
    uint8_t algorithm_;
    uint8_t feedback_;
    bool keyOnFlag_;
    float output_;
    float feedback_buffer_[2];
};

// YM2151チップクラス
class Chip {
public:
    Chip(uint32_t clock = 3579545);
    ~Chip();

    void reset();
    void setRegister(uint8_t reg, uint8_t value);
    uint8_t getRegister(uint8_t reg) const;
    void generate(float* buffer, int samples);
    
    // サンプリングレートの設定
    void setSampleRate(uint32_t rate);

    // チャンネルの取得
    Channel& getChannel(int index);

private:
    uint32_t clock_;
    uint32_t sample_rate_;
    std::array<uint8_t, REGISTER_COUNT> registers_;
    std::array<Channel, CHANNEL_COUNT> channels_;
    
    // 内部タイマー
    uint8_t timer_a_val_;
    uint8_t timer_b_val_;
    bool timer_a_enabled_;
    bool timer_b_enabled_;
    bool timer_a_overflow_;
    bool timer_b_overflow_;
    
    // LFO
    uint8_t lfo_frequency_;
    uint8_t lfo_waveform_;
    float lfo_phase_;
    float lfo_am_depth_;
    float lfo_pm_depth_;
    
    // 内部処理用
    void updateTimers();
    void updateLFO();
    float getLFOValue();
};

} // namespace YM2151

#endif // YM2151_H
