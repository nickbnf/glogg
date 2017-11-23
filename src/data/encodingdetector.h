#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H

#include <QMutex>

class QTextCodec;

class EncodingDetector {
public:

    static EncodingDetector& getInstance()
    {
        static auto * const instance = new EncodingDetector();
        return *instance;
    }

    EncodingDetector(const EncodingDetector&) = delete;
    EncodingDetector& operator=(const EncodingDetector&) = delete;
    EncodingDetector(const EncodingDetector&&) = delete;
    EncodingDetector& operator=(const EncodingDetector&&) = delete;


    QTextCodec* detectEncoding(const QByteArray& block) const;

  private:
    EncodingDetector() = default;
    ~EncodingDetector() = default;

    mutable QMutex mutex_;
};

#endif // ENCODINGDETECTOR_H
