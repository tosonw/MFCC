# 项目
MFCC

# 关键词
mfcc, mel, pcen, librosa.

# 说明
基于Python的librosa库里的mel特征提取和pcen特征提取，移植C++的实现。

对于一个2秒22050采样率的文件（不考虑文件加载）：

    耗时：11ms（首次运行，将初始化mel滤波器）
    耗时：7ms（之后的运行耗时）

原文链接：https://blog.csdn.net/Tosonw/article/details/98184666
1.基于该博客程序进行修改：https://blog.csdn.net/LiuPeiP_VIPL/article/details/81742392
2.根据Python平台librosa库的运算逻辑进行移植
3.使用NumCpp来实现Python平台的NumPy：https://github.com/dpilger26/NumCpp
4.本例中的FFT运算非常慢（约160ms），后来使用的是GitHub上找到的（约6ms）: https://github.com/HiFi-LoFi/AudioFFT
5.后来经过验证发现NumCpp效率比较低，于是使用opencv来实现矩阵运算。
6.后来优化使用pcen来实现mel特征提取，其中使用了IIR滤波器：https://blog.csdn.net/liyuanbhu/article/details/38849897

# 环境
Ubuntu 16.04LTS， Core-i7 8700， Clion
# 依赖
openCV
# 编译
mkdir build 

cd build

cmake ..

make
# 运行
./demo_mfcc_t
