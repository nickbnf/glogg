#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H

#include <QMutex>

class QTextCodec;

class EncodingDetector {
public:

    static EncodingDetector& getInstance()
    {
        static EncodingDetector * const instance = new EncodingDetector();
        return *instance;
    }

    QTextCodec* detectEncoding(const QByteArray& block) const;

  private:
    EncodingDetector() = default;
    ~EncodingDetector() = default;

    EncodingDetector(const EncodingDetector&) = delete;
    EncodingDetector& operator=(const EncodingDetector&) = delete;
    EncodingDetector(const EncodingDetector&&) = delete;
    EncodingDetector& operator=(const EncodingDetector&&) = delete;

    mutable QMutex mutex_;
};

#endif // ENCODINGDETECTOR_H
