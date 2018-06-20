#ifndef ENCODINGDETECTOR_H
#define ENCODINGDETECTOR_H

#include <QMutex>

class QTextCodec;

struct EncodingParameters
{
    EncodingParameters():lineFeedWidth(1),lineFeedIndex(0){}
    explicit EncodingParameters(const QTextCodec* codec);

    int lineFeedWidth;
    int lineFeedIndex;

    bool operator ==(const EncodingParameters& other) const
    {
        return  lineFeedWidth == other.lineFeedWidth &&
                lineFeedIndex == other.lineFeedIndex;
    }

    bool operator !=(const EncodingParameters& other) const
    {
        return !operator ==(other);
    }

    int getBeforeCrOffset() const
    {
        return lineFeedIndex;
    }

    int getAfterCrOffset() const
    {
        return lineFeedWidth - lineFeedIndex - 1;
    }
};

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
