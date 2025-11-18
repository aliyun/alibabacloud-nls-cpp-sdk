#include <stdint.h>
#include <string>
#include <vector>

std::string timestampStr(struct timeval* tv, uint64_t* tvUs);
uint64_t getTimestampUs(struct timeval* tv);
uint64_t getTimestampMs(struct timeval* tv);
std::string findConnectType(const char* input);
bool isNotEmptyAndNotSpace(const char* str);
std::vector<std::string> splitString(
    const std::string& str, const std::vector<std::string>& delimiters);
void findWavFiles(const std::string& directory,
                  std::vector<std::string>& wavFiles);
std::string getWavFile(std::vector<std::string>& wavFiles);
unsigned int getAudioFileTimeMs(const int dataSize, const int sampleRate,
                                const int compressRate);

/**
 * @brief 获取sendAudio发送延时时间
 * @param dataSize 待发送数据大小
 * @param sampleRate 采样率 16k/8K
 * @param compressRate 数据压缩率，例如压缩比为10:1的16k opus编码，此时为10；
 *                     非压缩数据则为1
 * @return 返回sendAudio之后需要sleep的时间
 * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
         对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100 ms.
         对于其它编码格式(OPUS)的数据, 由于传递给SDK的仍然是PCM编码数据,
         按照SDK OPUS/OPU 数据长度限制, 需要每次发送640字节 sleep 20ms.
 */
unsigned int getSendAudioSleepTime(const int dataSize, const int sampleRate,
                                   const int compressRate);

std::string createTranscriptionPathFromAudioPath(const std::string& audioPath);

void deleteFileIfExists(const std::string& filename);