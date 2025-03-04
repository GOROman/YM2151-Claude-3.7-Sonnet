# YM2151 FM音源エミュレータ

YM2151（OPM）FM音源チップのC++実装です。YM2151はYAMAHAが開発したFM音源チップで、多くのアーケードゲームや音楽機器で使用されていました。

## 特徴

- C++17で実装
- 8チャンネル、4オペレータのFM音源
- 8種類のアルゴリズム
- LFO（低周波発振器）サポート
- タイマー機能

## 必要条件

- C++17対応コンパイラ
- CMake 3.10以上

## ビルド方法

```bash
# ビルドディレクトリの作成
mkdir build
cd build

# CMakeの実行
cmake ..

# ビルド
cmake --build .
```

## 使用方法

### ライブラリとしての使用

```cpp
#include "ym2151/ym2151.h"

// YM2151チップの初期化
YM2151::Chip chip;
chip.reset();

// サンプリングレートの設定
chip.setSampleRate(44100);

// レジスタの設定
chip.setRegister(0x20, 0x04);  // チャンネル0のアルゴリズム4、フィードバック0

// 音声生成
float buffer[1024];
chip.generate(buffer, 1024);
```

### サンプルプログラム

`examples/simple_tone.cpp` は、YM2151を使用して単純な音色を生成し、WAVファイルとして保存するサンプルプログラムです。

```bash
# サンプルプログラムの実行
./simple_tone
```

これにより、`ym2151_tone.wav` というWAVファイルが生成されます。

## YM2151レジスタマップ

| アドレス | 説明 |
|---------|------|
| 0x01    | LFO周波数 |
| 0x08    | キーオン/オフ |
| 0x0F    | ノイズイネーブル、ノイズ周波数 |
| 0x10-0x17 | チャンネル周波数（下位8ビット） |
| 0x18-0x1F | チャンネル周波数（上位8ビット） |
| 0x20-0x27 | アルゴリズム、フィードバック |
| 0x28-0x2F | キースケール、オペレータマスク |
| 0x30-0x37 | LFO感度、PMS、AMS |
| 0x38-0x3F | 左/右出力 |
| 0x40-0x5F | デチューン、マルチプル |
| 0x60-0x7F | トータルレベル |
| 0x80-0x9F | キースケール、アタックレート |
| 0xA0-0xBF | AM感度、ディケイレート |
| 0xC0-0xDF | デチューン2、サスティンレート |
| 0xE0-0xFF | サスティンレベル、リリースレート |

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。詳細はLICENSEファイルを参照してください。

## 参考資料

- [YM2151データシート](https://www.vgmpf.com/Wiki/images/f/f6/YM2151_-_Manual.pdf)
- [OPM音源の仕組み](http://www.retropc.net/mm/sound/opm/index.htm)
