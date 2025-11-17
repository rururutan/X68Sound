#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
削除された日本語コメントを追加するスクリプト
"""

import re

# ファイルごとのコメント追加マッピング
file_comments = {
    'global.h': [
        (r'^int OpmWait = 240;$', 'int OpmWait = 240;\t// 24.0us相当'),
        (r'^int\s+OpmRate = 62500;$', 'int\tOpmRate = 62500;\t// 実機のクロックは64'),
        (r'^int\s+TotalVolume;$', 'int\tTotalVolume;\t// 全体音量 x/256'),
        (r'^static double opmlowpass_dummy_44;$', 'static double opmlowpass_dummy_44;\t// 64bit境界合わせ'),
        (r'^static double opmlowpass_dummy_48;$', 'static double opmlowpass_dummy_48;\t// 64bit境界合わせ'),
    ],
    'opm.h': [
        (r'^typedef HRESULT \(WINAPI \*C86CtlCreateInstance\)', '// c86ctl用定義\ntypedef HRESULT (WINAPI *C86CtlCreateInstance)'),
        (r'^\s+Op\s+op\[8\]\[4\];$', '\tOp\top[8][4];\t// オペレータ0～31'),
        (r'^\s+int\s+EnvCounter1;$', '\tint\tEnvCounter1;\t// エンベロープ用カウンタ1 (0,1,2,3,4,5,6,...)'),
        (r'^\s+int\s+EnvCounter2;$', '\tint\tEnvCounter2;\t// エンベロープ用カウンタ2 (3,2,1,3,2,1,3,2,...)'),
        (r'^\s+int\s+pan\[2\]\[N_CH\];$', '\tint\tpan[2][N_CH];\t// 0:無音 -1:出力'),
        (r'^\s+int\s+TimerAreg10;$', '\tint\tTimerAreg10;\t// OPMreg$10の値'),
        (r'^\s+int\s+TimerAreg11;$', '\tint\tTimerAreg11;\t// OPMreg$11の値'),
        (r'^\s+int\s+TimerA;$', '\tint\tTimerA;\t\t\t// タイマーAのオーバーフロー設定値'),
        (r'^\s+int\s+TimerAcounter;$', '\tint\tTimerAcounter;\t// タイマーAのカウンター値'),
        (r'^\s+int\s+TimerB;$', '\tint\tTimerB;\t\t\t// タイマーBのオーバーフロー設定値'),
        (r'^\s+int\s+TimerBcounter;$', '\tint\tTimerBcounter;\t// タイマーBのカウンター値'),
        (r'^\s+volatile int\s+TimerReg;$', '\tvolatile int\tTimerReg;\t\t// タイマー制御レジスタ (OPMreg$14の下位4ビット+7ビット)'),
        (r'^\s+volatile int\s+StatReg;$', '\tvolatile int\tStatReg;\t\t// OPMステータスレジスタ ($E90003の下位2ビット)'),
        (r'^\s+void \(CALLBACK \*OpmIntProc\)\(\);$', '\tvoid (CALLBACK *OpmIntProc)();\t// OPM割り込みコールバック関数'),
        (r'^\s+unsigned char\s+AdpcmBaseClock;$', '\tunsigned char\tAdpcmBaseClock;\t// ADPCMクロック切り替え(0:8MHz 1:4Mhz)'),
        (r'^\s+unsigned char\s+OpmRegNo;$', '\tunsigned char\tOpmRegNo;\t\t// 現在指定されているOPMレジスタ番号'),
        (r'^\s+unsigned char\s+OpmRegNo_backup;$', '\tunsigned char\tOpmRegNo_backup;\t\t// バックアップ用OPMレジスタ番号'),
        (r'^\s+void \(CALLBACK \*BetwIntProc\)\(\);$', '\tvoid (CALLBACK *BetwIntProc)();\t// マルチメディアタイマー割り込み'),
        (r'^\s+int\s+UseOpmFlag;$', '\tint\tUseOpmFlag;\t\t// OPMを利用するかどうかのフラグ'),
        (r'^\s+int\s+UseAdpcmFlag;$', '\tint\tUseAdpcmFlag;\t// ADPCMを利用するかどうかのフラグ'),
        (r'^\s+int Dousa_mode;$', '\tint Dousa_mode;\t\t// 0:非動作 1:X68Sound_Start中  2:X68Sound_PcmStart中'),
        (r'^\s+NumCmnd = 0;$', '\tNumCmnd = 0;\t// OPMコマンドバッファをクリア'),
    ],
    'adpcm.h': [
        (r'^\s+int\s+InpPcm,InpPcm_prev,OutPcm;$', '\tint\tInpPcm,InpPcm_prev,OutPcm;\t\t// HPF用 16bit PCM Data'),
        (r'^\s+int\s+OutInpPcm,OutInpPcm_prev;$', '\tint\tOutInpPcm,OutInpPcm_prev;\t\t// HPF用'),
        (r'^\s+int\s+N1Data;$', '\tint\tN1Data;\t// ADPCM 1サンプルのデータの保存'),
        (r'^\s+void \(CALLBACK \*IntProc\)\(\);$', '\tvoid (CALLBACK *IntProc)();\t// 割り込みアドレス'),
        (r'^\s+void \(CALLBACK \*ErrIntProc\)\(\);$', '\tvoid (CALLBACK *ErrIntProc)();\t// エラー割り込みアドレス'),
        (r'^\s+DmaError\(0x0D\);$', '\t\t\tDmaError(0x0D);\t// カウントエラー(転送先アドレス/転送先カウンタ)'),
        (r'^\s+DmaError\(0x0B\);$', '\t\t\tDmaError(0x0B);\t\t// バスエラー(ベースアドレス/ベースカウンタ)'),
        (r'^\s+DmaError\(0x09\);$', '\t\t\tDmaError(0x09);\t// バスエラー(転送先アドレス/転送先カウンタ)'),
        (r'^\s+if \(AdpcmReg & 0x80\)', '\tif (AdpcmReg & 0x80)\t\t// ADPCM 停止中'),
        (r'^\s+if \(N1DataFlag == 0\)', '\t\tif (N1DataFlag == 0)\t\t// 次のADPCMデータが必要になった場合'),
        (r'^\s+N10Data = DmaGetByte\(\);', '\t\t\tN10Data = DmaGetByte();\t// DMA転送(1バイト)'),
        (r'^\s+adpcm2pcm\(N10Data & 0x0F\);', '\t\t\tadpcm2pcm(N10Data & 0x0F);\t// InpPcm に値を代入'),
        (r'^\s+adpcm2pcm\(N1Data\);', '\t\t\tadpcm2pcm(N1Data);\t\t\t// InpPcm に値を代入'),
    ],
    'pcm8.h': [
        (r'^\s+int\s+Pcm16Prev;$', '\tint\tPcm16Prev;\t// 16bit,8bitPCMの1つ前のデータ'),
        (r'^\s+int\s+InpPcm,InpPcm_prev,OutPcm;$', '\tint\tInpPcm,InpPcm_prev,OutPcm;\t\t// HPF用 16bit PCM Data'),
        (r'^\s+int\s+OutInpPcm,OutInpPcm_prev;$', '\tint\tOutInpPcm,OutInpPcm_prev;\t\t// HPF用'),
        (r'^\s+volatile int\s+PcmKind;$', '\tvolatile int\tPcmKind;\t// 0～4:ADPCM  5:16bitPCM  6:8bitPCM  7:無効'),
        (r'^\s+volatile int\s+DmaOcr;$', '\tvolatile int\tDmaOcr;\t\t\t\t// 0:チェイニング設定なし 0x08:アレイチェイン 0x0C:リンクアレイチェイン'),
        (r'^\s+if \(AdpcmReg & 0x80\)', '\tif (AdpcmReg & 0x80)\t\t// ADPCM 停止中'),
        (r'^\s+AdpcmReg = 0xC7;', '\t\t\t\tAdpcmReg = 0xC7;\t// ADPCM 停止'),
        (r'^\s+if \(PcmKind == 5\)', '\t\tif (PcmKind == 5)\t// 16bitPCM'),
        (r'^\s+} else if \(PcmKind == 6\)', '\t\t} else if (PcmKind == 6)\t// 8bitPCM'),
        (r'^\s+if \(N1DataFlag == 0\)', '\t\t\tif (N1DataFlag == 0)\t\t// 次のADPCMデータが必要になった場合'),
        (r'^\s+N10Data = DmaGetByte\(\);', '\t\t\t\tN10Data = DmaGetByte();\t// DMA転送(1バイト)'),
        (r'^\s+adpcm2pcm\(N10Data & 0x0F\);', '\t\t\t\tadpcm2pcm(N10Data & 0x0F);\t// InpPcm に値を代入'),
        (r'^\s+adpcm2pcm\(N1Data\);', '\t\t\t\tadpcm2pcm(N1Data);\t\t\t// InpPcm に値を代入'),
        (r'^\s+pcm16_2pcm\(\(int\)\(short\)\(\(dataH<<8\)\|dataL\)\);', '\t\t\tpcm16_2pcm((int)(short)((dataH<<8)|dataL));\t// OutPcm に値を代入'),
        (r'^\s+pcm16_2pcm\(\(int\)\(char\)data\);', '\t\t\tpcm16_2pcm((int)(char)data);\t// InpPcm に値を代入'),
    ],
}

def add_comments(filepath, patterns):
    """ファイルに適切な日本語コメントを追加"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        original = content
        for pattern, replacement in patterns:
            content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

        if content != original:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
        return False
    except Exception as e:
        print(f"Error adding comments to {filepath}: {e}")
        return False

if __name__ == '__main__':
    import os
    base_path = '/home/user/X68Sound2025/X68Sound/'

    fixed_count = 0
    for filename, patterns in file_comments.items():
        filepath = os.path.join(base_path, filename)
        if os.path.exists(filepath):
            if add_comments(filepath, patterns):
                print(f"Added comments to: {filename}")
                fixed_count += 1

    print(f"\nTotal files with added comments: {fixed_count}")
