//
// Created by toson on 19-7-17.
//

#include "utils/mfcc.hpp"
#include "utils/sas_util.h"
#include "fstream"

#if 0 //没有使用加载wav的程序，而是直接加载保存好的二进制数据
typedef struct wave_tag {
    char ChunkID[4];                    // "RIFF"标志
    unsigned int ChunkSize;             // 文件长度(WAVE文件的大小, 不含前8个字节)
    char Format[4];                     // "WAVE"标志
    char SubChunk1ID[4];                // "fmt "标志
    unsigned int SubChunk1Size;         // 过渡字节(不定)
    unsigned short int AudioFormat;     // 格式类别(10H为PCM格式的声音数据)
    unsigned short int NumChannels;     // 通道数(单声道为1, 双声道为2)
    unsigned int SampleRate;            // 采样率(每秒样本数), 表示每个通道的播放速度
    unsigned int ByteRate;              // 波形音频数据传输速率, 其值为:通道数*每秒数据位数*每样本的数据位数/8
    unsigned short int BlockAlign;      // 每样本的数据位数(按字节算), 其值为:通道数*每样本的数据位值/8
    unsigned short int BitsPerSample;   // 每样本的数据位数, 表示每个声道中各个样本的数据位数.
    char SubChunk2ID[4];                // 数据标记"data"
    unsigned int SubChunk2Size;         // 语音数据的长度

} waveft;

int resampleData(const int16_t *sourceData, int32_t sampleRate, uint32_t srcSize, int16_t *destinationData,
                 int32_t newSampleRate, uint32_t dstSize = 0) {
    if (sampleRate == newSampleRate) {
        memcpy(destinationData, sourceData, srcSize * sizeof(int16_t));
        return 0;
    }
    uint32_t last_pos = srcSize - 1;
    if (dstSize != (uint32_t) (srcSize * ((float) newSampleRate / sampleRate))) {
        return -1;
    }
    int s = 0;
    for (uint32_t idx = 0; idx < dstSize; idx++) {
        float index = ((float) idx * sampleRate) / (newSampleRate);
        uint32_t p1 = (uint32_t) index;
        float coef = index - p1;
        uint32_t p2 = (p1 == last_pos) ? last_pos : p1 + 1;
        destinationData[idx] = (int16_t) ((1.0f - coef) * sourceData[p1] + coef * sourceData[p2]);
    }
    return 0;
}

class Wav {
private:
    char *buffer8;
    short *buffer16;
    float *buffer32;

public:
    unsigned char *srcdata;         //data中储存音频的有效数据
    unsigned char *resampled_data;  //重采样过后的数据
    waveft waveformatex;
    int length_buffer;
    int length_wav;//音频有效数据的长度(采样点数)
    int thislabel;

    Wav(const char *path, unsigned int nSamplesPerSec) {
        //创建对象，并初始化对象的变量值
        memset(&waveformatex, 0, sizeof(waveformatex));
        length_buffer = 0;
        length_wav = 0;

        buffer8 = new char[length_buffer];
        buffer16 = new short[length_buffer];
        buffer32 = new float[length_buffer];
        srcdata = new unsigned char[length_wav];
        resampled_data = new unsigned char[length_wav];
        thislabel = Load(path, nSamplesPerSec);
    }

    void WavToBuffer() {
        switch (waveformatex.BitsPerSample) {
            case 8:
                buffer8 = (char *) realloc(buffer8, sizeof(char) * (length_buffer = length_wav));

                for (int i = 0; i < length_buffer; i++) {
                    buffer8[i] = (char) resampled_data[i];
                }
                break;
            case 16:
                buffer16 = (short *) realloc(buffer16, sizeof(short) * (length_buffer = length_wav / 2));

                for (int i = 0; i < length_buffer; i++) {
                    buffer16[i] = (short) ((resampled_data[2 * i + 1] << 8) | resampled_data[2 * i]);
                }
                break;
            case 32:
                buffer32 = (float *) realloc(buffer32, sizeof(float) * (length_buffer = length_wav / 4));

                for (int i = 0; i < length_buffer; i++) {
                    buffer32[i] = *(float *) &resampled_data[4 * i];
                }
                break;
        }
    }

    int Load(const char *path, unsigned int nSamplesPerSec) {
        FILE *file = fopen(path, "rb");
        if (!file) {
            fprintf(stderr, "[Load] [%s not found]\n", path);
            return -1;
        }
        int chunk = 0;
        fread(&waveformatex, sizeof(struct wave_tag), 1, file);
        std::cout << "wav file length: " << waveformatex.ChunkSize << " bytes" << std::endl;
        //std::cout << "waveformatex.SubChunk1Size: " << waveformatex.SubChunk1Size << " bytes" << std::endl;
        //std::cout << "waveformatex.NumChannels: " << waveformatex.NumChannels << std::endl;
        std::cout << "nSamplesPerSec: " << waveformatex.SampleRate << " Hz" << std::endl;
        //std::cout << "waveformatex.BitsPerSample: " << waveformatex.BitsPerSample << " bits" << std::endl;
        fread(&chunk, 2, 1, file);
        while (chunk != 0x6164 && fread(&chunk, 2, 1, file) != EOF);
        fread(&chunk, 2, 1, file);
        if (chunk != 0x6174) { //检索“data”字符: 0x64617461
            return -1;
        }
        fread(&length_wav, sizeof(int), 1, file);
        srcdata = (unsigned char *) realloc(srcdata, length_wav);//申请length_wav大小的空间，空间的首地址为data
        std::cout << "length_buffer = length_wav: " << length_wav << " bytes" << std::endl;
        fread(srcdata, length_wav, 1, file);//将音频的有效数据存储到data中。
        fclose(file);

        //重采样
        if (nSamplesPerSec != waveformatex.SampleRate) {
            uint32_t dstSize = (uint32_t) (length_wav * ((float) nSamplesPerSec / waveformatex.SampleRate));
            resampled_data = (unsigned char *) realloc(resampled_data, dstSize);
            if (!resampleData((int16_t *) srcdata, waveformatex.SampleRate, length_wav / 2,
                              (int16_t *) resampled_data, nSamplesPerSec, dstSize / 2)) {
                waveformatex.SampleRate = nSamplesPerSec;
                length_wav = dstSize;
            }
        } else {
            resampled_data = srcdata;
        }

        if (waveformatex.BitsPerSample != 32 || waveformatex.NumChannels != 1 ||
            waveformatex.SampleRate != nSamplesPerSec)
            return 1;
        else
            return 0;
    }

    double Get_Buffer(int index) {
        if (0 <= index && index < length_buffer) {
            switch (waveformatex.BitsPerSample) {
                case 8:
                    return (buffer8[index] + 0.5) / 127.5;
                case 16:
                    return (buffer16[index] + 0.5) / 32767.5;
                case 32:
                    return buffer32[index];
            }
        }
        return 0;
    }

};
#endif

int main() {
//    // 读取文件
//    Wav wav("raw.wav", nSamplesPerSec);
//    wav.WavToBuffer();
//    if (wav.thislabel == 1) {
//        std::cout << "wav.thislabel == 1" << std::endl;
//        return -1;
//    }
//    std::cout << "SampleRate: " << wav.waveformatex.SampleRate << " Hz" << std::endl;

    //我读取的wav数据和Python(librosa)读出来的不一样，深入检查未排查到原因。
    // 于是我将python里读取的wav数据保存了下来（张尚龙python代码读取的wav数据）
    // 现在载入该数据：
    unsigned long ifile_length = 0;
    std::vector<uint8_t> ifile_data; //wav数据
    std::fstream ifile("/home/toson/CLionProjects/MFCC/y.bin", std::ios::in);//使用python导出的wav数据
    if (ifile) {
        ifile.seekg(0, std::ios::end);                  // go to the end
        ifile_length = (unsigned long) ifile.tellg();   // report location (this is the length)
        ifile.seekg(0, std::ios::beg);                  // go back to the beginning
        ifile_data.resize(ifile_length, 0);
        ifile.read((char *) ifile_data.data(), ifile_length);
        ifile.close();
    } else {
        printf("It is no file: y.bin\n");
        return -1;
    }

    auto stime = NowTime<milliseconds>();
    // 运算mel特征向量 //11ms
    //auto mel = log_mel(ifile_data, 16000);
    auto mel = pcen(ifile_data, 16000);
    auto etime = NowTime<milliseconds>();
    std::cout << "all cost: " << etime - stime << std::endl;

    std::cout << "[" << mel.rows << ", " << mel.cols << "]" << std::endl;
//    cv::print(mel);

    // 首次运算会多使用5ms，生成mel滤波器用。
    //auto mel2 = log_mel(ifile_data, 16000);
    auto mel2 = pcen(ifile_data, 16000);
    auto etime2 = NowTime<milliseconds>();
    std::cout << "all2 cost: " << etime2 - etime << std::endl;
    cv::print(mel2);
//    for (int j = 0; j < mel2.rows; j++) {
//        for (int i = 0; i < mel2.cols; i++) {
//            std::cout << "[ " << j << " , " << i << " ]: " << mel2(j, i) << std::endl;
//        }
//    }
//    std::cout << std::endl;

    //保存为文件
//    std::fstream ofile("out_mel.csv", std::ios::out);
//    if (ofile) {
//        for (int i = 0; i < mel.shape().rows; i++) {
//            for (int j = 0; j < mel.shape().cols; j++) {
//                ofile << mel(i, j) << " ";
//            }
//            ofile << std::endl;
//        }
//        ofile.close();
//    }
//    std::cout << R"(Save as "out_mel.csv")" << std::endl;

    return 0;
}
